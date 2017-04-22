/* gtd-provider-local.c
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

#include "gtd-provider-local.h"
#include "gtd-task-list-eds.h"

#include <glib/gi18n.h>

struct _GtdProviderLocal
{
  GtdProviderEds          parent;

  GIcon                  *icon;
  GList                  *tasklists;
};

static void          gtd_provider_iface_init                     (GtdProviderInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GtdProviderLocal, gtd_provider_local, GTD_TYPE_PROVIDER_EDS,
                         G_IMPLEMENT_INTERFACE (GTD_TYPE_PROVIDER,
                                                gtd_provider_iface_init))

enum {
  PROP_0,
  PROP_ENABLED,
  PROP_DEFAULT_TASKLIST,
  PROP_DESCRIPTION,
  PROP_ICON,
  PROP_ID,
  PROP_NAME,
  LAST_PROP
};

/*
 * GtdProviderInterface implementation
 */
static const gchar*
gtd_provider_local_get_id (GtdProvider *provider)
{
  return "local";
}

static const gchar*
gtd_provider_local_get_name (GtdProvider *provider)
{
  return _("Local");
}

static const gchar*
gtd_provider_local_get_description (GtdProvider *provider)
{
  return _("On This Computer");
}


static gboolean
gtd_provider_local_get_enabled (GtdProvider *provider)
{
  return TRUE;
}

static GIcon*
gtd_provider_local_get_icon (GtdProvider *provider)
{
  GtdProviderLocal *self;

  self = GTD_PROVIDER_LOCAL (provider);

  return self->icon;
}

static const GtkWidget*
gtd_provider_local_get_edit_panel (GtdProvider *provider)
{
  return NULL;
}

static void
gtd_provider_local_create_task (GtdProvider *provider,
                                GtdTask     *task)
{
  gtd_provider_eds_create_task (GTD_PROVIDER_EDS (provider), task);
}

static void
gtd_provider_local_update_task (GtdProvider *provider,
                                GtdTask     *task)
{
  gtd_provider_eds_update_task (GTD_PROVIDER_EDS (provider), task);
}

static void
gtd_provider_local_remove_task (GtdProvider *provider,
                                GtdTask     *task)
{
  gtd_provider_eds_remove_task (GTD_PROVIDER_EDS (provider), task);
}

static void
gtd_provider_local_create_task_list (GtdProvider *provider,
                                     GtdTaskList *list)
{
  ESourceExtension *extension;
  GtdTaskListEds *eds_list;
  ESource *source;
  GError *error;

  error = NULL;

  /* Create the source */
  source = e_source_new (NULL,
                         NULL,
                         &error);

  if (error)
    {
      g_warning ("%s: %s: %s",
                 G_STRFUNC,
                 _("Error creating new task list"),
                 error->message);

      gtd_manager_emit_error_message (gtd_manager_get_default (),
                                      _("Error creating new task list"),
                                      error->message);

      g_clear_error (&error);
      return;
    }

  /* EDS properties */
  e_source_set_display_name (source, gtd_task_list_get_name (list));

  /* Make it a local source */
  extension = e_source_get_extension (source, E_SOURCE_EXTENSION_TASK_LIST);

  e_source_set_parent (source, "local-stub");
  e_source_backend_set_backend_name (E_SOURCE_BACKEND (extension), "local");

  /* EDS Task list */
  eds_list = gtd_task_list_eds_new (provider, source);

  gtd_provider_eds_create_task_list (GTD_PROVIDER_EDS (provider), GTD_TASK_LIST (eds_list));
}

static void
gtd_provider_local_update_task_list (GtdProvider *provider,
                                     GtdTaskList *list)
{
  gtd_provider_eds_update_task_list (GTD_PROVIDER_EDS (provider), list);

  g_signal_emit_by_name (provider, "list-changed", list);
}

static void
gtd_provider_local_remove_task_list (GtdProvider *provider,
                                     GtdTaskList *list)
{
  gtd_provider_eds_remove_task_list (GTD_PROVIDER_EDS (provider), list);

  g_signal_emit_by_name (provider, "list-removed", list);
}

static GList*
gtd_provider_local_get_task_lists (GtdProvider *provider)
{
  return gtd_provider_eds_get_task_lists (GTD_PROVIDER_EDS (provider));
}

static GtdTaskList*
gtd_provider_local_get_default_task_list (GtdProvider *provider)
{
  return gtd_provider_eds_get_default_task_list (GTD_PROVIDER_EDS (provider));
}

static void
gtd_provider_local_set_default_task_list (GtdProvider *provider,
                                          GtdTaskList *list)
{
  gtd_provider_eds_set_default_task_list (GTD_PROVIDER_EDS (provider), list);
}

static void
gtd_provider_iface_init (GtdProviderInterface *iface)
{
  iface->get_id = gtd_provider_local_get_id;
  iface->get_name = gtd_provider_local_get_name;
  iface->get_description = gtd_provider_local_get_description;
  iface->get_enabled = gtd_provider_local_get_enabled;
  iface->get_icon = gtd_provider_local_get_icon;
  iface->get_edit_panel = gtd_provider_local_get_edit_panel;
  iface->create_task = gtd_provider_local_create_task;
  iface->update_task = gtd_provider_local_update_task;
  iface->remove_task = gtd_provider_local_remove_task;
  iface->create_task_list = gtd_provider_local_create_task_list;
  iface->update_task_list = gtd_provider_local_update_task_list;
  iface->remove_task_list = gtd_provider_local_remove_task_list;
  iface->get_task_lists = gtd_provider_local_get_task_lists;
  iface->get_default_task_list = gtd_provider_local_get_default_task_list;
  iface->set_default_task_list = gtd_provider_local_set_default_task_list;
}

GtdProviderLocal*
gtd_provider_local_new (ESourceRegistry *registry)
{
  return g_object_new (GTD_TYPE_PROVIDER_LOCAL,
                       "registry", registry,
                       NULL);
}

static void
gtd_provider_local_finalize (GObject *object)
{
  GtdProviderLocal *self = (GtdProviderLocal *)object;

  g_clear_object (&self->icon);

  G_OBJECT_CLASS (gtd_provider_local_parent_class)->finalize (object);
}

static void
gtd_provider_local_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GtdProvider *provider = GTD_PROVIDER (object);

  switch (prop_id)
    {
    case PROP_DEFAULT_TASKLIST:
      g_value_set_object (value, gtd_provider_local_get_default_task_list (provider));
      break;

    case PROP_DESCRIPTION:
      g_value_set_string (value, gtd_provider_local_get_description (provider));
      break;

    case PROP_ENABLED:
      g_value_set_boolean (value, gtd_provider_local_get_enabled (provider));
      break;

    case PROP_ICON:
      g_value_set_object (value, gtd_provider_local_get_icon (provider));
      break;

    case PROP_ID:
      g_value_set_string (value, gtd_provider_local_get_id (provider));
      break;

    case PROP_NAME:
      g_value_set_string (value, gtd_provider_local_get_name (provider));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtd_provider_local_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GtdProvider *provider = GTD_PROVIDER (object);

  switch (prop_id)
    {
    case PROP_DEFAULT_TASKLIST:
      gtd_provider_local_set_default_task_list (provider, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static gboolean
gtd_provider_local_should_load_source (GtdProviderEds *provider,
                                       ESource        *source)
{
  if (e_source_has_extension (source, E_SOURCE_EXTENSION_TASK_LIST))
    return g_strcmp0 (e_source_get_parent (source), "local-stub") == 0;

  return FALSE;
}

static void
gtd_provider_local_class_init (GtdProviderLocalClass *klass)
{
  GtdProviderEdsClass *eds_class = GTD_PROVIDER_EDS_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  eds_class->should_load_source = gtd_provider_local_should_load_source;

  object_class->finalize = gtd_provider_local_finalize;
  object_class->get_property = gtd_provider_local_get_property;
  object_class->set_property = gtd_provider_local_set_property;

  g_object_class_override_property (object_class, PROP_DEFAULT_TASKLIST, "default-task-list");
  g_object_class_override_property (object_class, PROP_DESCRIPTION, "description");
  g_object_class_override_property (object_class, PROP_ENABLED, "enabled");
  g_object_class_override_property (object_class, PROP_ICON, "icon");
  g_object_class_override_property (object_class, PROP_ID, "id");
  g_object_class_override_property (object_class, PROP_NAME, "name");
}

static void
gtd_provider_local_init (GtdProviderLocal *self)
{
  gtd_object_set_ready (GTD_OBJECT (self), TRUE);

  /* icon */
  self->icon = G_ICON (g_themed_icon_new_with_default_fallbacks ("computer-symbolic"));
}
