/* gtd-provider-eds.c
 *
 * Copyright (C) 2015 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gtd-provider-eds.h"
#include "gtd-task-list-eds.h"

#include <glib/gi18n.h>

/**
 * #GtdProviderEds is the base class of #GtdProviderLocal
 * and #GtdProviderGoa. It provides the common functionality
 * shared between these two providers.
 *
 * The subclasses basically have to implement GtdProviderEds->should_load_source
 * which decides whether a given #ESource should be loaded (and added to the
 * sources list) or not. #GtdProviderLocal for example would filter out
 * sources whose backend is not "local".
 */

typedef struct
{
  GList                *task_lists;

  ESourceRegistry      *source_registry;
  ECredentialsPrompter *credentials_prompter;

  GHashTable           *clients;

  gint                  lazy_load_id;
} GtdProviderEdsPrivate;

/* Auxiliary struct for asyncronous task operations */
typedef struct _TaskData
{
  GtdProviderEds     *provider;
  gpointer           *data;
} TaskData;


G_DEFINE_TYPE_WITH_PRIVATE (GtdProviderEds, gtd_provider_eds, GTD_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_REGISTRY,
  N_PROPS
};

static TaskData*
task_data_new (GtdProviderEds *provider,
               gpointer       *data)
{
  TaskData *tdata;

  tdata = g_new0 (TaskData, 1);
  tdata->provider = provider;
  tdata->data = data;

  return tdata;
}

static void
gtd_provider_eds_set_default (GtdProviderEds *self,
                              GtdTaskList    *list)
{
  GtdProviderEdsPrivate *priv;
  ESource *source;

  priv = gtd_provider_eds_get_instance_private (self);
  source = gtd_task_list_eds_get_source (GTD_TASK_LIST_EDS (list));

  gtd_manager_set_default_provider (gtd_manager_get_default (), GTD_PROVIDER (self));
  e_source_registry_set_default_task_list (priv->source_registry, source);
}

static void
gtd_provider_eds_fill_task_list (GObject      *client,
                                 GAsyncResult *result,
                                 gpointer      user_data)
{
  GtdTaskList *list;
  TaskData *data = user_data;
  GSList *component_list;
  GError *error = NULL;

  g_return_if_fail (GTD_IS_PROVIDER_EDS (data->provider));

  list = GTD_TASK_LIST (data->data);

  e_cal_client_get_object_list_as_comps_finish (E_CAL_CLIENT (client),
                                                result,
                                                &component_list,
                                                &error);

  gtd_object_set_ready (GTD_OBJECT (data->data), TRUE);
  g_free (data);

  if (!error)
    {
      GSList *l;

      for (l = component_list; l != NULL; l = l->next)
        {
          GtdTask *task;

          task = gtd_task_new (l->data);
          gtd_task_set_list (task, list);

          gtd_task_list_save_task (list, task);
        }

      e_cal_client_free_ecalcomp_slist (component_list);
    }
  else
    {
      g_warning ("%s: %s: %s",
                 G_STRFUNC,
                 _("Error fetching tasks from list"),
                 error->message);

      gtd_manager_emit_error_message (gtd_manager_get_default (),
                                      _("Error fetching tasks from list"),
                                      error->message);
      g_error_free (error);
      return;
    }
}

static void
gtd_provider_eds_on_client_connected (GObject      *source_object,
                                      GAsyncResult *result,
                                      gpointer      user_data)
{
  GtdProviderEdsPrivate *priv;
  GtdProviderEds *self;
  GtdTaskListEds *list;
  ECalClient *client;
  TaskData *data;
  ESource *default_source;
  ESource *source;
  ESource *parent;
  GError *error;

  self = GTD_PROVIDER_EDS (user_data);
  priv = gtd_provider_eds_get_instance_private (self);
  error = NULL;
  source = e_client_get_source (E_CLIENT (source_object));
  client = E_CAL_CLIENT (e_cal_client_connect_finish (result, &error));

  if (error)
    {
      g_debug ("%s: %s (%s): %s",
               G_STRFUNC,
               _("Failed to connect to task list"),
               e_source_get_uid (source),
               error->message);

      gtd_manager_emit_error_message (gtd_manager_get_default (),
                                      _("Failed to connect to task list"),
                                      error->message);

      g_error_free (error);
      return;
    }

  /* parent source's display name is list's origin */
  parent = e_source_registry_ref_source (priv->source_registry, e_source_get_parent (source));

  /* creates a new task list */
  list = gtd_task_list_eds_new (GTD_PROVIDER (self), source);

  /* it's not ready until we fetch the list of tasks from client */
  gtd_object_set_ready (GTD_OBJECT (list), FALSE);

  /* async data */
  data = task_data_new (user_data, (gpointer) list);

  /* asyncronously fetch the task list */
  e_cal_client_get_object_list_as_comps (client,
                                         "contains? \"any\" \"\"",
                                         NULL,
                                         gtd_provider_eds_fill_task_list,
                                         data);

  priv->task_lists = g_list_append (priv->task_lists, list);

  g_object_set_data (G_OBJECT (source), "task-list", list);
  g_hash_table_insert (priv->clients, source, client);

  /* Check if the current list is the default one */
  default_source = e_source_registry_ref_default_task_list (priv->source_registry);

  if (default_source == source)
    g_object_notify (G_OBJECT (self), "default-task-list");

  g_clear_object (&default_source);

  /* Emit LIST_ADDED signal */
  g_signal_emit_by_name (self, "list-added", list);

  g_object_unref (parent);

  g_debug ("%s: %s (%s)",
           G_STRFUNC,
           _("Task list source successfully connected"),
           e_source_get_display_name (source));
}

typedef struct
{
  GtdProviderEds *provider;
  ESource        *source;
} LoadSourceData;

static gboolean
gtd_provider_eds_load_source_cb (LoadSourceData *data)
{
  GtdProviderEdsPrivate *priv;
  GtdProviderEds *provider;
  ESource *source;

  provider = data->provider;
  source = data->source;
  priv = gtd_provider_eds_get_instance_private (provider);

  if (e_source_has_extension (source, E_SOURCE_EXTENSION_TASK_LIST) &&
      !g_hash_table_lookup (priv->clients, source) &&
      GTD_PROVIDER_EDS_CLASS (G_OBJECT_GET_CLASS (provider))->should_load_source (provider, source))
    {
      e_cal_client_connect (source,
                            E_CAL_CLIENT_SOURCE_TYPE_TASKS,
                            10, /* seconds to wait */
                            NULL,
                            gtd_provider_eds_on_client_connected,
                            provider);
    }

  g_free (data);

  return G_SOURCE_REMOVE;
}

static void
gtd_provider_eds_load_source (GtdProviderEds *provider,
                              ESource        *source)
{
  LoadSourceData *data;

  data = g_new0 (LoadSourceData, 1);
  data->provider = provider;
  data->source = source;

  /* HACK: I really don't like to use arbitrary timeouts on
   * my code, but we have absolutely no guarantees that
   * ESourceRegistry::source-added was emited to the other
   * objects before. So Milan Crha told me to add this timeout
   * and "guarantee" that other objects will receive the
   * signal.
   */
  g_timeout_add (1000,
                 (GSourceFunc) gtd_provider_eds_load_source_cb,
                 data);
}

static void
gtd_provider_eds_remove_source (GtdProviderEds *provider,
                                ESource        *source)
{
  GtdProviderEdsPrivate *priv;
  GtdTaskList *list;

  priv = gtd_provider_eds_get_instance_private (provider);
  list = g_object_get_data (G_OBJECT (source), "task-list");

  priv->task_lists = g_list_remove (priv->task_lists, list);

  g_hash_table_remove (priv->clients, source);

  /* Since all subclasses will have this signal given that they
   * are all GtdProvider implementations, it's not that bad
   * to let it stay here.
   */
  g_signal_emit_by_name (provider, "list-removed", list);
}




/************************
 * Credentials prompter *
 ************************/

static void
gtd_manager__invoke_authentication (GObject      *source_object,
                                    GAsyncResult *result,
                                    gpointer      user_data)
{
  ESource *source = E_SOURCE (source_object);
  GError *error = NULL;
  gboolean canceled;

  e_source_invoke_authenticate_finish (source,
                                       result,
                                       &error);

  canceled = g_error_matches (error,
                                   G_IO_ERROR,
                                   G_IO_ERROR_CANCELLED);

  if (!canceled)
    {
      g_warning ("%s: %s (%s): %s",
                 G_STRFUNC,
                 _("Failed to prompt for credentials"),
                 e_source_get_uid (source),
                 error->message);
    }

  g_clear_error (&error);
}

static void
gtd_provider_local_credentials_prompt_done (GObject      *source_object,
                                            GAsyncResult *result,
                                            gpointer      user_data)
{
  ETrustPromptResponse response = E_TRUST_PROMPT_RESPONSE_UNKNOWN;
  ESource *source = E_SOURCE (source_object);
  GError *error = NULL;

  e_trust_prompt_run_for_source_finish (source, result, &response, &error);

  if (error)
    {
      g_warning ("%s: %s '%s': %s",
                 G_STRFUNC,
                 _("Failed to prompt for credentials for"),
                 e_source_get_display_name (source),
                 error->message);

    }
  else if (response == E_TRUST_PROMPT_RESPONSE_ACCEPT || response == E_TRUST_PROMPT_RESPONSE_ACCEPT_TEMPORARILY)
    {
      /* Use NULL credentials to reuse those from the last time. */
      e_source_invoke_authenticate (source,
                                    NULL,
                                    NULL /* cancellable */,
                                    gtd_manager__invoke_authentication,
                                    NULL);
    }

  g_clear_error (&error);
}

static void
gtd_provider_eds_credentials_required (ESourceRegistry          *registry,
                                       ESource                  *source,
                                       ESourceCredentialsReason  reason,
                                       const gchar              *certificate_pem,
                                       GTlsCertificateFlags      certificate_errors,
                                       const GError             *error,
                                       gpointer                  user_data)
{
  GtdProviderEdsPrivate *priv;
  GtdProviderEds *self;

  g_return_if_fail (GTD_IS_PROVIDER_EDS (user_data));

  self = GTD_PROVIDER_EDS (user_data);
  priv = gtd_provider_eds_get_instance_private (self);

  if (e_credentials_prompter_get_auto_prompt_disabled_for (priv->credentials_prompter, source))
    return;

  if (reason == E_SOURCE_CREDENTIALS_REASON_SSL_FAILED)
    {
      e_trust_prompt_run_for_source (e_credentials_prompter_get_dialog_parent (priv->credentials_prompter),
                                     source,
                                     certificate_pem,
                                     certificate_errors,
                                     error ? error->message : NULL,
                                     TRUE, // allow saving sources
                                     NULL, // we won't cancel the operation
                                     gtd_provider_local_credentials_prompt_done,
                                     NULL);
    }
  else if (error && reason == E_SOURCE_CREDENTIALS_REASON_ERROR)
    {
      g_warning ("%s: %s '%s': %s",
                 G_STRFUNC,
                 _("Authentication failure"),
                 e_source_get_display_name (source),
                 error->message);
    }
}


static void
gtd_provider_eds_finalize (GObject *object)
{
  GtdProviderEds *self = (GtdProviderEds *)object;
  GtdProviderEdsPrivate *priv = gtd_provider_eds_get_instance_private (self);

  g_clear_pointer (&priv->clients, g_hash_table_destroy);
  g_clear_object (&priv->credentials_prompter);
  g_clear_object (&priv->source_registry);

  G_OBJECT_CLASS (gtd_provider_eds_parent_class)->finalize (object);
}

static void
gtd_provider_eds_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GtdProviderEds *self = GTD_PROVIDER_EDS (object);
  GtdProviderEdsPrivate *priv = gtd_provider_eds_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_REGISTRY:
      g_value_set_object (value, priv->source_registry);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtd_provider_eds_load_registry (GtdProviderEds  *provider)
{
  GtdProviderEdsPrivate *priv = gtd_provider_eds_get_instance_private (provider);
  GList *sources;
  GList *l;
  GError *error = NULL;

  priv->credentials_prompter = e_credentials_prompter_new (priv->source_registry);

  if (error != NULL)
    {
      g_warning ("%s: %s", _("Error loading task manager"), error->message);
      g_error_free (error);
      return;
    }

  /* First of all, disable authentication dialog for non-tasklists sources */
  sources = e_source_registry_list_sources (priv->source_registry, NULL);

  for (l = sources; l != NULL; l = g_list_next (l))
    {
      ESource *source = E_SOURCE (l->data);

      /* Mark for skip also currently disabled sources */
      e_credentials_prompter_set_auto_prompt_disabled_for (priv->credentials_prompter,
                                                           source,
                                                           !e_source_has_extension (source, E_SOURCE_EXTENSION_TASK_LIST));
    }

  g_list_free_full (sources, g_object_unref);

  /* Load task list sources */
  sources = e_source_registry_list_sources (priv->source_registry, E_SOURCE_EXTENSION_TASK_LIST);

  for (l = sources; l != NULL; l = l->next)
    gtd_provider_eds_load_source (provider, l->data);

  g_list_free_full (sources, g_object_unref);

  /* listen to the signals, so new sources don't slip by */
  g_signal_connect_swapped (priv->source_registry,
                            "source-added",
                            G_CALLBACK (gtd_provider_eds_load_source),
                            provider);

  g_signal_connect_swapped (priv->source_registry,
                            "source-removed",
                            G_CALLBACK (gtd_provider_eds_remove_source),
                            provider);

  g_signal_connect (priv->source_registry,
                    "credentials-required",
                    G_CALLBACK (gtd_provider_eds_credentials_required),
                    provider);

  e_credentials_prompter_process_awaiting_credentials (priv->credentials_prompter);
}

static gboolean
gtd_provider_eds_try_load (GtdProviderEds *provider)
{
  GtdProviderEdsPrivate *priv = gtd_provider_eds_get_instance_private (provider);

  if (gtd_object_get_ready (GTD_OBJECT (provider)))
    {
      gtd_provider_eds_load_registry (provider);
      priv->lazy_load_id = 0;

      return G_SOURCE_REMOVE;
    }

  return G_SOURCE_CONTINUE;
}

static void
gtd_provider_eds_set_registry (GtdProviderEds  *provider,
                               ESourceRegistry *registry)
{
  GtdProviderEdsPrivate *priv = gtd_provider_eds_get_instance_private (provider);

  g_set_object (&priv->source_registry, registry);

  priv->lazy_load_id = g_timeout_add (250,
                                      (GSourceFunc) gtd_provider_eds_try_load,
                                      provider);
}


static void
gtd_provider_eds_create_task_finished (GObject      *client,
                                       GAsyncResult *result,
                                       gpointer      user_data)
{
  TaskData *data = user_data;
  gchar *new_uid = NULL;
  GError *error = NULL;

  e_cal_client_create_object_finish (E_CAL_CLIENT (client),
                                     result,
                                     &new_uid,
                                     &error);

  gtd_object_set_ready (GTD_OBJECT (data->data), TRUE);

  if (error)
    {
      GtdTask *task = GTD_TASK (data->data);

      g_warning ("%s: %s: %s",
                 G_STRFUNC,
                 _("Error creating task"),
                 error->message);

      /* Remove from the tasklist */
      gtd_task_list_remove_task (gtd_task_get_list (task), task);

      /* Display a notification */
      gtd_manager_emit_error_message (gtd_manager_get_default (),
                                      _("Error creating task"),
                                      error->message);

      g_error_free (error);
    }
  else
    {
      /* Update the default tasklist */
      gtd_provider_eds_set_default (data->provider, gtd_task_get_list (GTD_TASK (data->data)));

      /*
       * In the case the task UID changes because of creation proccess,
       * reapply it to the task.
       */
      if (new_uid)
        {
          gtd_object_set_uid (GTD_OBJECT (data->data), new_uid);
          g_free (new_uid);
        }
    }

  g_free (data);
}

static void
gtd_provider_eds_update_task_finished (GObject      *client,
                                       GAsyncResult *result,
                                       gpointer      user_data)
{
  TaskData *data = user_data;
  GError *error = NULL;

  e_cal_client_modify_object_finish (E_CAL_CLIENT (client),
                                     result,
                                     &error);

  if (error)
    {
      g_warning ("%s: %s: %s",
                 G_STRFUNC,
                 _("Error updating task"),
                 error->message);

      gtd_manager_emit_error_message (gtd_manager_get_default (),
                                      _("Error updating task"),
                                      error->message);

      g_error_free (error);
    }

  gtd_object_set_ready (GTD_OBJECT (data->data), TRUE);

  g_free (data);
}

static void
gtd_provider_eds_remove_task_finished (GObject      *client,
                                       GAsyncResult *result,
                                       gpointer      user_data)
{
  TaskData *data = user_data;
  GError *error = NULL;

  e_cal_client_remove_object_finish (E_CAL_CLIENT (client),
                                     result,
                                     &error);

  gtd_object_set_ready (GTD_OBJECT (data->data), TRUE);

  g_object_unref ((GtdTask*) data->data);
  g_free (data);

  if (error)
    {
      g_warning ("%s: %s: %s",
                 G_STRFUNC,
                 _("Error removing task"),
                 error->message);

      gtd_manager_emit_error_message (gtd_manager_get_default (),
                                      _("Error removing task"),
                                      error->message);
      g_error_free (error);
      return;
    }
}

static void
gtd_provider_eds_remote_create_source_finished (GObject      *source,
                                                GAsyncResult *result,
                                                gpointer      user_data)
{
  GError *error;

  error = NULL;

  e_source_remote_create_finish (E_SOURCE (source),
                                 result,
                                 &error);

  if (error)
    {
      g_warning ("%s: %s: %s",
                 G_STRFUNC,
                 _("Error creating task list"),
                 error->message);

      gtd_manager_emit_error_message (gtd_manager_get_default (),
                                      _("Error creating task list"),
                                      error->message);
      g_clear_error (&error);
    }
}

static void
task_list_removal_finished (GtdProvider *provider,
                            ESource     *source,
                            GError     **error)
{
  gtd_object_set_ready (GTD_OBJECT (provider), TRUE);

  if (*error)
    {
      g_warning ("%s: %s: %s",
                 G_STRFUNC,
                 _("Error removing task list"),
                 (*error)->message);

      gtd_manager_emit_error_message (gtd_manager_get_default (),
                                      _("Error removing task list"),
                                      (*error)->message);
      g_clear_error (error);
    }
}


static void
gtd_provider_eds_remote_delete_finished (GObject      *source,
                                         GAsyncResult *result,
                                         gpointer      user_data)
{
  GError *error = NULL;

  e_source_remote_delete_finish (E_SOURCE (source),
                                 result,
                                 &error);

  task_list_removal_finished (GTD_PROVIDER (user_data),
                              E_SOURCE (source),
                              &error);
}

static void
gtd_provider_eds_remove_source_finished (GObject      *source,
                                         GAsyncResult *result,
                                         gpointer      user_data)
{
  GError *error = NULL;

  e_source_remove_finish (E_SOURCE (source),
                          result,
                          &error);

  task_list_removal_finished (GTD_PROVIDER (user_data),
                              E_SOURCE (source),
                              &error);
}

static void
gtd_provider_eds_commit_source_finished (GObject      *registry,
                                         GAsyncResult *result,
                                         gpointer      user_data)
{
  GError *error = NULL;

  g_return_if_fail (GTD_IS_PROVIDER_EDS (user_data));

  gtd_object_set_ready (GTD_OBJECT (user_data), TRUE);
  e_source_registry_commit_source_finish (E_SOURCE_REGISTRY (registry),
                                          result,
                                          &error);

  if (error)
    {
      g_warning ("%s: %s: %s",
                 G_STRFUNC,
                 _("Error saving task list"),
                 error->message);

      gtd_manager_emit_error_message (gtd_manager_get_default (),
                                      _("Error saving task list"),
                                      error->message);
      g_error_free (error);
      return;
    }
}

static void
gtd_provider_eds_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GtdProviderEds *self = GTD_PROVIDER_EDS (object);

  switch (prop_id)
    {
    case PROP_REGISTRY:
      gtd_provider_eds_set_registry (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtd_provider_eds_class_init (GtdProviderEdsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gtd_provider_eds_finalize;
  object_class->get_property = gtd_provider_eds_get_property;
  object_class->set_property = gtd_provider_eds_set_property;

  g_object_class_install_property (object_class,
                                   PROP_REGISTRY,
                                   g_param_spec_object ("registry",
                                                        "Source registry",
                                                        "The EDS source registry object",
                                                        E_TYPE_SOURCE_REGISTRY,
                                                        G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
gtd_provider_eds_init (GtdProviderEds *self)
{
  GtdProviderEdsPrivate *priv = gtd_provider_eds_get_instance_private (self);

  /* While it's not ready, we don't load tasklists */
  gtd_object_set_ready (GTD_OBJECT (self), FALSE);

  /* hash table */
  priv->clients = g_hash_table_new_full ((GHashFunc) e_source_hash,
                                         (GEqualFunc) e_source_equal,
                                         g_object_unref,
                                         g_object_unref);
}

GtdProviderEds*
gtd_provider_eds_new (ESourceRegistry *registry)
{
  return g_object_new (GTD_TYPE_PROVIDER_EDS,
                       "registry", registry,
                       NULL);
}

ESourceRegistry*
gtd_provider_eds_get_registry (GtdProviderEds *provider)
{
  GtdProviderEdsPrivate *priv;

  g_return_val_if_fail (GTD_IS_PROVIDER_EDS (provider), NULL);

  priv = gtd_provider_eds_get_instance_private (provider);

  return priv->source_registry;
}

void
gtd_provider_eds_create_task (GtdProviderEds *provider,
                              GtdTask        *task)
{
  GtdProviderEdsPrivate *priv;
  GtdTaskListEds *tasklist;
  ECalComponent *component;
  ECalClient *client;
  TaskData *data;
  ESource *source;

  g_return_if_fail (GTD_IS_TASK (task));
  g_return_if_fail (GTD_IS_TASK_LIST_EDS (gtd_task_get_list (task)));

  priv = gtd_provider_eds_get_instance_private (provider);
  tasklist = GTD_TASK_LIST_EDS (gtd_task_get_list (task));
  source = gtd_task_list_eds_get_source (tasklist);
  client = g_hash_table_lookup (priv->clients, source);
  component = gtd_task_get_component (task);

  /* Temporary data for async operation */
  data = task_data_new (provider, (gpointer) task);

  /* The task is not ready until we finish the operation */
  gtd_object_set_ready (GTD_OBJECT (task), FALSE);

  e_cal_client_create_object (client,
                              e_cal_component_get_icalcomponent (component),
                              NULL, // We won't cancel the operation
                              (GAsyncReadyCallback) gtd_provider_eds_create_task_finished,
                              data);
}

void
gtd_provider_eds_update_task (GtdProviderEds *provider,
                              GtdTask        *task)
{
  GtdProviderEdsPrivate *priv;
  GtdTaskListEds *tasklist;
  ECalComponent *component;
  ECalClient *client;
  TaskData *data;
  ESource *source;

  g_return_if_fail (GTD_IS_TASK (task));
  g_return_if_fail (GTD_IS_TASK_LIST_EDS (gtd_task_get_list (task)));

  priv = gtd_provider_eds_get_instance_private (provider);
  tasklist = GTD_TASK_LIST_EDS (gtd_task_get_list (task));
  source = gtd_task_list_eds_get_source (tasklist);
  client = g_hash_table_lookup (priv->clients, source);
  component = gtd_task_get_component (task);

  /* Temporary data for async operation */
  data = task_data_new (provider, (gpointer) task);

  /* The task is not ready until we finish the operation */
  gtd_object_set_ready (GTD_OBJECT (task), FALSE);

  e_cal_client_modify_object (client,
                              e_cal_component_get_icalcomponent (component),
                              E_CAL_OBJ_MOD_THIS,
                              NULL, // We won't cancel the operation
                              (GAsyncReadyCallback) gtd_provider_eds_update_task_finished,
                              data);
}

void
gtd_provider_eds_remove_task (GtdProviderEds *provider,
                              GtdTask        *task)
{

  GtdProviderEdsPrivate *priv;
  ECalComponent *component;
  GtdTaskListEds *tasklist;
  ECalComponentId *id;
  ECalClient *client;
  TaskData *data;
  ESource *source;

  g_return_if_fail (GTD_IS_TASK (task));
  g_return_if_fail (GTD_IS_TASK_LIST_EDS (gtd_task_get_list (task)));

  priv = gtd_provider_eds_get_instance_private (provider);
  tasklist = GTD_TASK_LIST_EDS (gtd_task_get_list (task));
  source = gtd_task_list_eds_get_source (tasklist);
  client = g_hash_table_lookup (priv->clients, source);
  component = gtd_task_get_component (task);
  id = e_cal_component_get_id (component);

  /* Temporary data for async operation */
  data = task_data_new (provider, (gpointer) task);

  /* The task is not ready until we finish the operation */
  gtd_object_set_ready (GTD_OBJECT (task), FALSE);

  e_cal_client_remove_object (client,
                              id->uid,
                              id->rid,
                              E_CAL_OBJ_MOD_THIS,
                              NULL, // We won't cancel the operation
                              (GAsyncReadyCallback) gtd_provider_eds_remove_task_finished,
                              data);

  e_cal_component_free_id (id);
}

void
gtd_provider_eds_create_task_list (GtdProviderEds *provider,
                                   GtdTaskList    *list)
{
  GtdProviderEdsPrivate *priv;
  ESource *source;

  g_return_if_fail (GTD_IS_TASK_LIST_EDS (list));
  g_return_if_fail (gtd_task_list_eds_get_source (GTD_TASK_LIST_EDS (list)));

  priv = gtd_provider_eds_get_instance_private (provider);
  source = gtd_task_list_eds_get_source (GTD_TASK_LIST_EDS (list));

  gtd_object_set_ready (GTD_OBJECT (provider), FALSE);
  e_source_registry_commit_source (priv->source_registry,
                                   source,
                                   NULL,
                                   (GAsyncReadyCallback) gtd_provider_eds_commit_source_finished,
                                   provider);

  if (e_source_get_remote_creatable (source))
    {
      ESource *parent;

      parent = e_source_registry_ref_source (priv->source_registry, e_source_get_parent (source));

      e_source_remote_create (parent,
                              source,
                              NULL,
                              (GAsyncReadyCallback) gtd_provider_eds_remote_create_source_finished,
                              provider);

      g_object_unref (parent);
    }
}

void
gtd_provider_eds_update_task_list (GtdProviderEds *provider,
                                   GtdTaskList    *list)
{

  GtdProviderEdsPrivate *priv;
  ESource *source;

  g_return_if_fail (GTD_IS_TASK_LIST (list));
  g_return_if_fail (gtd_task_list_eds_get_source (GTD_TASK_LIST_EDS (list)));

  priv = gtd_provider_eds_get_instance_private (provider);
  source = gtd_task_list_eds_get_source (GTD_TASK_LIST_EDS (list));

  gtd_object_set_ready (GTD_OBJECT (provider), FALSE);
  e_source_registry_commit_source (priv->source_registry,
                                   source,
                                   NULL,
                                   (GAsyncReadyCallback) gtd_provider_eds_commit_source_finished,
                                   provider);
}

void
gtd_provider_eds_remove_task_list (GtdProviderEds *provider,
                                   GtdTaskList    *list)
{
  ESource *source;

  g_return_if_fail (GTD_IS_TASK_LIST (list));
  g_return_if_fail (gtd_task_list_eds_get_source (GTD_TASK_LIST_EDS (list)));

  source = gtd_task_list_eds_get_source (GTD_TASK_LIST_EDS (list));

  gtd_object_set_ready (GTD_OBJECT (provider), FALSE);

  if (e_source_get_remote_deletable (source))
    {
      e_source_remote_delete (source,
                              NULL,
                              (GAsyncReadyCallback) gtd_provider_eds_remote_delete_finished,
                              provider);
    }
  else
    {
      e_source_remove (source,
                       NULL,
                       (GAsyncReadyCallback) gtd_provider_eds_remove_source_finished,
                       provider);
    }
}

GList*
gtd_provider_eds_get_task_lists (GtdProviderEds *provider)
{
  GtdProviderEdsPrivate *priv = gtd_provider_eds_get_instance_private (provider);

  return priv->task_lists;
}

GtdTaskList*
gtd_provider_eds_get_default_task_list (GtdProviderEds *provider)
{
  GtdProviderEdsPrivate *priv;
  GtdTaskList *default_task_list;
  ESource *default_source;

  priv = gtd_provider_eds_get_instance_private (provider);
  default_source = e_source_registry_ref_default_task_list (priv->source_registry);
  default_task_list = g_object_get_data (G_OBJECT (default_source), "task-list");

  g_clear_object (&default_source);

  if (default_task_list &&
      gtd_task_list_get_provider (default_task_list) != GTD_PROVIDER (provider))
    {
      return NULL;
    }

  return default_task_list;
}

void
gtd_provider_eds_set_default_task_list (GtdProviderEds *provider,
                                        GtdTaskList    *list)
{
  GtdProviderEdsPrivate *priv;
  ESource *source;

  priv = gtd_provider_eds_get_instance_private (provider);
  source = g_object_get_data (G_OBJECT (list), "task-list");

  e_source_registry_set_default_task_list (priv->source_registry, source);

  g_object_notify (G_OBJECT (provider), "default-task-list");
}
