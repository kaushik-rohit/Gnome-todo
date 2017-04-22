/* gtd-provider-goa.c
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
#include "gtd-provider-goa.h"

#include <glib/gi18n.h>

struct _GtdProviderGoa
{
  GtdProviderEds          parent;

  GoaAccount             *account;
  GIcon                  *icon;

  gchar                  *id;
};

static void          gtd_provider_iface_init                     (GtdProviderInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GtdProviderGoa, gtd_provider_goa, GTD_TYPE_PROVIDER_EDS,
                         G_IMPLEMENT_INTERFACE (GTD_TYPE_PROVIDER,
                                                gtd_provider_iface_init))

enum {
  PROP_0,
  PROP_ACCOUNT,
  PROP_DEFAULT_TASKLIST,
  PROP_DESCRIPTION,
  PROP_ENABLED,
  PROP_ICON,
  PROP_ID,
  PROP_NAME,
  N_PROPS
};

/*
 * GtdProviderInterface implementation
 */
static const gchar*
gtd_provider_goa_get_id (GtdProvider *provider)
{
  GtdProviderGoa *self;

  self = GTD_PROVIDER_GOA (provider);

  return self->id;
}

static const gchar*
gtd_provider_goa_get_name (GtdProvider *provider)
{
  GtdProviderGoa *self;

  self = GTD_PROVIDER_GOA (provider);

  return goa_account_get_provider_name (self->account);
}

static const gchar*
gtd_provider_goa_get_description (GtdProvider *provider)
{
  GtdProviderGoa *self;

  self = GTD_PROVIDER_GOA (provider);

  return goa_account_get_identity (self->account);
}


static gboolean
gtd_provider_goa_get_enabled (GtdProvider *provider)
{
  GtdProviderGoa *self;

  self = GTD_PROVIDER_GOA (provider);

  return !goa_account_get_calendar_disabled (self->account);
}

static GIcon*
gtd_provider_goa_get_icon (GtdProvider *provider)
{
  GtdProviderGoa *self;

  self = GTD_PROVIDER_GOA (provider);

  return self->icon;
}

static const GtkWidget*
gtd_provider_goa_get_edit_panel (GtdProvider *provider)
{
  return NULL;
}

static void
gtd_provider_goa_create_task (GtdProvider *provider,
                              GtdTask     *task)
{
  gtd_provider_eds_create_task (GTD_PROVIDER_EDS (provider), task);
}

static void
gtd_provider_goa_update_task (GtdProvider *provider,
                              GtdTask     *task)
{
  gtd_provider_eds_update_task (GTD_PROVIDER_EDS (provider), task);
}

static void
gtd_provider_goa_remove_task (GtdProvider *provider,
                              GtdTask     *task)
{
  gtd_provider_eds_remove_task (GTD_PROVIDER_EDS (provider), task);
}

static void
gtd_provider_goa_create_task_list (GtdProvider *provider,
                                   GtdTaskList *list)
{
  gtd_provider_eds_create_task_list (GTD_PROVIDER_EDS (provider), list);

  g_signal_emit_by_name (provider, "list-added", list);
}

static void
gtd_provider_goa_update_task_list (GtdProvider *provider,
                                   GtdTaskList *list)
{
  gtd_provider_eds_update_task_list (GTD_PROVIDER_EDS (provider), list);

  g_signal_emit_by_name (provider, "list-changed", list);
}

static void
gtd_provider_goa_remove_task_list (GtdProvider *provider,
                                   GtdTaskList *list)
{
  gtd_provider_eds_remove_task_list (GTD_PROVIDER_EDS (provider), list);

  g_signal_emit_by_name (provider, "list-removed", list);
}

static GList*
gtd_provider_goa_get_task_lists (GtdProvider *provider)
{
  return gtd_provider_eds_get_task_lists (GTD_PROVIDER_EDS (provider));
}

static GtdTaskList*
gtd_provider_goa_get_default_task_list (GtdProvider *provider)
{
  return gtd_provider_eds_get_default_task_list (GTD_PROVIDER_EDS (provider));
}

static void
gtd_provider_goa_set_default_task_list (GtdProvider *provider,
                                        GtdTaskList *list)
{
  gtd_provider_eds_set_default_task_list (GTD_PROVIDER_EDS (provider), list);
}

static void
gtd_provider_iface_init (GtdProviderInterface *iface)
{
  iface->get_id = gtd_provider_goa_get_id;
  iface->get_name = gtd_provider_goa_get_name;
  iface->get_description = gtd_provider_goa_get_description;
  iface->get_enabled = gtd_provider_goa_get_enabled;
  iface->get_icon = gtd_provider_goa_get_icon;
  iface->get_edit_panel = gtd_provider_goa_get_edit_panel;
  iface->create_task = gtd_provider_goa_create_task;
  iface->update_task = gtd_provider_goa_update_task;
  iface->remove_task = gtd_provider_goa_remove_task;
  iface->create_task_list = gtd_provider_goa_create_task_list;
  iface->update_task_list = gtd_provider_goa_update_task_list;
  iface->remove_task_list = gtd_provider_goa_remove_task_list;
  iface->get_task_lists = gtd_provider_goa_get_task_lists;
  iface->get_default_task_list = gtd_provider_goa_get_default_task_list;
  iface->set_default_task_list = gtd_provider_goa_set_default_task_list;
}

static void
gtd_provider_goa_set_account (GtdProviderGoa *provider,
                              GoaAccount     *account)
{
  gtd_object_set_ready (GTD_OBJECT (provider), account != NULL);

  if (provider->account != account)
    {
      gchar *icon_name;

      g_set_object (&provider->account, account);
      g_object_notify (G_OBJECT (provider), "account");


      g_message ("Setting up Online Account: %s (%s)",
                 goa_account_get_identity (account),
                 goa_account_get_id (account));

      /* Update icon */
      icon_name = g_strdup_printf ("goa-account-%s", goa_account_get_provider_type (provider->account));
      g_set_object (&provider->icon, g_themed_icon_new (icon_name));
      g_object_notify (G_OBJECT (provider), "icon");

      /* Provider id */
      provider->id = g_strdup_printf ("%s@%s",
                                      goa_account_get_provider_type (provider->account),
                                      goa_account_get_id (provider->account));

      g_free (icon_name);
    }
}

static void
gtd_provider_goa_finalize (GObject *object)
{
  GtdProviderGoa *self = (GtdProviderGoa *)object;

  g_clear_pointer (&self->id, g_free);

  g_clear_object (&self->account);
  g_clear_object (&self->icon);

  G_OBJECT_CLASS (gtd_provider_goa_parent_class)->finalize (object);
}

static void
gtd_provider_goa_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GtdProviderGoa *self = GTD_PROVIDER_GOA (object);
  GtdProvider *provider = GTD_PROVIDER (object);

  switch (prop_id)
    {

    case PROP_ACCOUNT:
      g_value_set_object (value, self->account);
      break;

    case PROP_DEFAULT_TASKLIST:
      g_value_set_object (value, gtd_provider_goa_get_default_task_list (provider));
      break;

    case PROP_DESCRIPTION:
      g_value_set_string (value, gtd_provider_goa_get_description (provider));
      break;

    case PROP_ENABLED:
      g_value_set_boolean (value, gtd_provider_goa_get_enabled (provider));
      break;

    case PROP_ICON:
      g_value_set_object (value, gtd_provider_goa_get_icon (provider));
      break;

    case PROP_ID:
      g_value_set_string (value, gtd_provider_goa_get_id (provider));
      break;

    case PROP_NAME:
      g_value_set_string (value, gtd_provider_goa_get_name (provider));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtd_provider_goa_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GtdProviderGoa *self = GTD_PROVIDER_GOA (object);

  switch (prop_id)
    {
    case PROP_ACCOUNT:
      gtd_provider_goa_set_account (self, g_value_get_object (value));
      break;

    case PROP_DEFAULT_TASKLIST:
      gtd_provider_goa_set_default_task_list (GTD_PROVIDER (self), g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static gboolean
gtd_provider_goa_should_load_source (GtdProviderEds *provider,
                                     ESource        *source)
{
  GtdProviderGoa *self;
  gboolean retval;

  self = GTD_PROVIDER_GOA (provider);
  retval = FALSE;

  if (e_source_has_extension (source, E_SOURCE_EXTENSION_TASK_LIST))
    {
      ESource *ancestor;

      ancestor = e_source_registry_find_extension (gtd_provider_eds_get_registry (provider),
                                                   source,
                                                   E_SOURCE_EXTENSION_GOA);

      /*
       * If we detect that the given source is provided
       * by a GOA account, check the account id.
       */
      if (ancestor)
        {
          ESourceExtension *extension;
          const gchar *ancestor_id;
          const gchar *account_id;

          extension = e_source_get_extension (ancestor, E_SOURCE_EXTENSION_GOA);
          ancestor_id = e_source_goa_get_account_id (E_SOURCE_GOA (extension));
          account_id = goa_account_get_id (self->account);

          /*
           * When the ancestor's GOA id matches the current
           * account's id, we shall load this list.
           */
          retval = g_strcmp0 (ancestor_id, account_id) == 0;
        }
    }

  return retval;
}

static void
gtd_provider_goa_class_init (GtdProviderGoaClass *klass)
{
  GtdProviderEdsClass *eds_class = GTD_PROVIDER_EDS_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  eds_class->should_load_source = gtd_provider_goa_should_load_source;

  object_class->finalize = gtd_provider_goa_finalize;
  object_class->get_property = gtd_provider_goa_get_property;
  object_class->set_property = gtd_provider_goa_set_property;

  g_object_class_override_property (object_class, PROP_DEFAULT_TASKLIST, "default-task-list");
  g_object_class_override_property (object_class, PROP_DESCRIPTION, "description");
  g_object_class_override_property (object_class, PROP_ENABLED, "enabled");
  g_object_class_override_property (object_class, PROP_ICON, "icon");
  g_object_class_override_property (object_class, PROP_ID, "id");
  g_object_class_override_property (object_class, PROP_NAME, "name");

  g_object_class_install_property (object_class,
                                   PROP_ACCOUNT,
                                   g_param_spec_object ("account",
                                                        "Account of the provider",
                                                        "The Online Account of the provider",
                                                        GOA_TYPE_ACCOUNT,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
gtd_provider_goa_init (GtdProviderGoa *self)
{
}

GtdProviderGoa*
gtd_provider_goa_new (ESourceRegistry *registry,
                      GoaAccount      *account)
{
  return g_object_new (GTD_TYPE_PROVIDER_GOA,
                       "account", account,
                       "registry", registry,
                       NULL);
}

GoaAccount*
gtd_provider_goa_get_account (GtdProviderGoa *provider)
{
  return provider->account;
}
