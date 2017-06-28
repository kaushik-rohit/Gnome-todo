/* gtd-provider-todoist.c
 *
 * Copyright (C) 2017 Rohit Kaushik <kaushikrohit325@gmail.com>
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

#include "gtd-provider-todoist.h"
#include "gtd-plugin-todoist.h"
#include <rest/oauth2-proxy.h>
#include <json-glib/json-glib.h>
#include <string.h>
#include <stdlib.h>

#include <glib/gi18n.h>


struct _GtdProviderTodoist
{
  GtdObject          parent;

  GoaObject          *account_object;

  gchar              *sync_token;
  gchar              *description;
  GIcon              *icon;

  GHashTable         *lists;
};

static void          gtd_provider_iface_init                     (GtdProviderInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GtdProviderTodoist, gtd_provider_todoist, GTD_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GTD_TYPE_PROVIDER,
                                                gtd_provider_iface_init))

enum {
  PROP_0,
  PROP_DEFAULT_TASKLIST,
  PROP_DESCRIPTION,
  PROP_ENABLED,
  PROP_ICON,
  PROP_ID,
  PROP_NAME,
  PROP_GOA_OBJECT,
  LAST_PROP
};

static const gchar *colormap[] =
{
  "#95ef63",
  "#ff8581",
  "#ffc471",
  "#f9ec75",
  "#a8c8e4",
  "#d2b8a3",
  "#e2a8e4",
  "#cccccc",
  "#fb886e",
  "#ffcc00",
  "#74e8d3",
  "#3bd5fb",
  "#dc4fad",
  "#ac193d",
  "#d24726",
  "#82ba00",
  "#03b3b2",
  "#008299",
  "#5db2ff",
  "#0072c6",
  "#000000",
  "#777777"
};

/*
 * GtdProviderInterface implementation
 */
static const gchar*
gtd_provider_todoist_get_id (GtdProvider *provider)
{
  return "todoist";
}

static const gchar*
gtd_provider_todoist_get_name (GtdProvider *provider)
{
  return _("Todoist");
}

static const gchar*
gtd_provider_todoist_get_description (GtdProvider *provider)
{
  GtdProviderTodoist *self;

  self = GTD_PROVIDER_TODOIST (provider);

  return self->description;
}


static gboolean
gtd_provider_todoist_get_enabled (GtdProvider *provider)
{
  return TRUE;
}

static GIcon*
gtd_provider_todoist_get_icon (GtdProvider *provider)
{
  GtdProviderTodoist *self;

  self = GTD_PROVIDER_TODOIST (provider);

  return self->icon;
}

static const GtkWidget*
gtd_provider_todoist_get_edit_panel (GtdProvider *provider)
{
  return NULL;
}

GoaObject*
gtd_provider_todoist_get_goa_object (GtdProviderTodoist  *self)
{
  return self->account_object;
}

static gint
optimized_eucledian_color_distance (GdkRGBA *color1,
                                    GdkRGBA *color2)
{
  gdouble red_diff;
  gdouble green_diff;
  gdouble blue_diff;
  gdouble red_mean_level;

  red_mean_level = (color1->red + color2->red) / 2;
  red_diff = color1->red - color2->red;
  green_diff = color1->green - color2->green;
  blue_diff = color1->blue - color2->blue;

  return (red_diff * red_diff * (2 + red_mean_level) +
          green_diff * green_diff * 4 +
          blue_diff * blue_diff * ((1 - red_mean_level) + 2));
}

static GdkRGBA*
convert_color_code (gint index)
{
  GdkRGBA rgba;

  gdk_rgba_parse (&rgba, colormap [index]);

  return gdk_rgba_copy (&rgba);
}

static void
emit_generic_error (GError *error)
{
  g_warning ("%s: %s: %s",
             G_STRFUNC,
             _("Error making a sync call to Todoist"),
             error->message);

  gtd_manager_emit_error_message (gtd_manager_get_default (),
                                  _("Error making a sync call to Todoist"),
                                  error->message,
                                  NULL,
                                  NULL);
}

static gint
get_color_code_index (GdkRGBA *rgba)
{
  guint nearest_color_index;
  guint min_color_diff;
  guint i;

  min_color_diff = G_MAXUINT;

  for (i = 0; i < G_N_ELEMENTS (colormap); i++)
    {
      GdkRGBA color;
      guint distance;

      gdk_rgba_parse (&color, colormap [i]);

      distance = optimized_eucledian_color_distance (rgba, &color);

      if (min_color_diff > distance)
        {
          min_color_diff = distance;
          nearest_color_index = i;
        }
    }

  return nearest_color_index;
}

static void
parse_array_to_list (GtdProviderTodoist *self,
                     JsonArray          *projects)
{
  GList *lists;
  GList *l;

  lists = NULL;
  l = NULL;

  lists = json_array_get_elements (projects);

  for (l = lists; l != NULL; l = l->next)
    {
      JsonObject *object;
      GtdTaskList *list;
      const gchar *name;
      gchar *uid;
      guint32 id;
      guint color_index;

      object = json_node_get_object (l->data);
      list = gtd_task_list_new (GTD_PROVIDER (self));

      if (json_object_has_member (object, "name"))
        name = json_object_get_string_member (object, "name");

      if (json_object_has_member (object, "color"))
        color_index = json_object_get_int_member (object, "color");

      if (json_object_has_member (object, "id"))
        id = json_object_get_int_member (object, "id");

      uid = g_strdup_printf ("%u", id);

      gtd_task_list_set_name (list, name);
      gtd_task_list_set_color (list, convert_color_code (color_index));
      gtd_object_set_uid (GTD_OBJECT (list), uid);
      g_hash_table_insert (self->lists, GUINT_TO_POINTER(id), list);

      g_free (uid);
    }
}

static void
parse_array_to_task (GtdProviderTodoist *self,
                     JsonArray          *items)
{
  GList *lists;
  GList *l;

  lists = NULL;
  l = NULL;

  lists = json_array_get_elements (items);

  for (l = lists; l != NULL; l = l->next)
    {
      JsonObject *object;
      GtdTaskList *list;
      ECalComponent *component;
      GtdTask *task;
      const gchar *title;
      gchar *uid;
      guint id;
      guint32 project_id;
      gint priority;

      component = e_cal_component_new ();
      e_cal_component_set_new_vtype (component, E_CAL_COMPONENT_TODO);
      e_cal_component_set_uid (component, e_cal_component_gen_uid ());

      task = gtd_task_new (component);

      object = json_node_get_object (l->data);

      if (json_object_has_member (object, "content"))
        title = json_object_get_string_member (object, "content");

      if (json_object_has_member (object, "priority"))
        priority = json_object_get_int_member (object, "priority");

      if (json_object_has_member (object, "id"))
        id = json_object_get_int_member (object, "id");

      if (json_object_has_member (object, "project_id"))
        project_id = json_object_get_int_member (object, "project_id");

      list = g_hash_table_lookup (self->lists, GUINT_TO_POINTER(project_id));
      uid = g_strdup_printf ("%d", id);
      gtd_object_set_uid (GTD_OBJECT (task), uid);
      gtd_task_set_title (task, title);
      gtd_task_set_list (task, list);
      gtd_task_set_priority (task, priority);
      gtd_task_list_save_task (list, task);

      g_free (uid);
    }
}

static JsonObject*
gtd_provider_todoist_sync_call (GtdProviderTodoist *self)
{
  RestProxy     *proxy;
  RestProxyCall *call;
  GoaOAuth2Based *o_auth2;
  JsonParser *parser;
  JsonObject *object;
  GError *error;
  GError *parse_error;
  const gchar *payload;
  gchar *access_token;
  guint status_code;
  gsize payload_length;

  error = NULL;
  parse_error = NULL;
  access_token = NULL;
  o_auth2 = goa_object_get_oauth2_based (self->account_object);
  proxy = rest_proxy_new ("https://todoist.com/API/v7/sync", FALSE);
  call = rest_proxy_new_call (proxy);
  parser = json_parser_new ();

  if (!goa_oauth2_based_call_get_access_token_sync (o_auth2, &access_token, NULL, NULL, &error))
    {
      emit_generic_error (error);
      g_clear_error (&error);
      goto out;
    }

  rest_proxy_call_set_method (call, "POST");
  rest_proxy_call_add_header (call, "content-type", "application/x-www-form-urlencoded");
  rest_proxy_call_add_param (call, "token", access_token);

  if (self->sync_token)
    rest_proxy_call_add_param (call, "sync_token", self->sync_token);
  else
    rest_proxy_call_add_param (call, "sync_token", "*");

  rest_proxy_call_add_param (call, "resource_types", "[\"all\"]");

  if (!rest_proxy_call_sync (call, &error))
    {
      emit_generic_error (error);
      g_clear_error (&error);
      goto out;
    }

  status_code = rest_proxy_call_get_status_code (call);

  if (status_code != 200)
    {
      gtd_manager_emit_error_message (gtd_manager_get_default (),
                                      _("Wrong status code recieved. Request to Todoist not performend correctly."),
                                      NULL,
                                      NULL,
                                      NULL);
      goto out;
    }

  payload = rest_proxy_call_get_payload (call);
  payload_length = rest_proxy_call_get_payload_length (call);

  if (!json_parser_load_from_data (parser, payload, payload_length, &parse_error))
    {
      emit_generic_error (parse_error);
      g_clear_error (&parse_error);
      goto out;
    }

  object = json_node_dup_object (json_parser_get_root (parser));

  if (json_object_has_member (object, "sync_token"))
    self->sync_token = g_strdup (json_object_get_string_member (object, "sync_token"));

out:
  g_object_unref (o_auth2);
  g_object_unref (parser);
  g_object_unref (proxy);
  g_object_unref (call);

  return object;
}

static void
load_tasks (GtdProviderTodoist *self)
{
  JsonObject *object;
  JsonArray *projects;
  JsonArray *items;

  object = gtd_provider_todoist_sync_call (self);

  if (json_object_has_member (object, "projects"))
    projects = json_object_get_array_member (object, "projects");
  if (json_object_has_member (object, "items"))
    items = json_object_get_array_member (object, "items");

  parse_array_to_list (self, projects);
  parse_array_to_task (self, items);
}

static void
gtd_provider_todoist_create_task (GtdProvider *provider,
                                   GtdTask     *task)
{

}

static void
gtd_provider_todoist_update_task (GtdProvider *provider,
                                   GtdTask     *task)
{

}

static void
gtd_provider_todoist_remove_task (GtdProvider *provider,
                                   GtdTask     *task)
{

}

static void
gtd_provider_todoist_create_task_list (GtdProvider *provider,
                                        GtdTaskList *list)
{

}

static void
gtd_provider_todoist_update_task_list (GtdProvider *provider,
                                        GtdTaskList *list)
{

}

static void
gtd_provider_todoist_remove_task_list (GtdProvider *provider,
                                        GtdTaskList *list)
{

}

static GList*
gtd_provider_todoist_get_task_lists (GtdProvider *provider)
{
  GtdProviderTodoist *self;

  self = GTD_PROVIDER_TODOIST (provider);

  return g_hash_table_get_values (self->lists);
}

static GtdTaskList*
gtd_provider_todoist_get_default_task_list (GtdProvider *provider)
{
  return NULL;
}

static void
gtd_provider_todoist_set_default_task_list (GtdProvider *provider,
                                            GtdTaskList *list)
{

}

static void
gtd_provider_todoist_set_description (GtdProviderTodoist *self)
{
  GoaAccount *account;
  gchar *identity;

  account = goa_object_get_account (self->account_object);
  identity = goa_account_dup_identity (account);
  self->description = g_strconcat ("Todoist: ", identity , NULL);

  g_object_unref (account);
  g_free (identity);
}

static void
gtd_provider_iface_init (GtdProviderInterface *iface)
{
  iface->get_id = gtd_provider_todoist_get_id;
  iface->get_name = gtd_provider_todoist_get_name;
  iface->get_description = gtd_provider_todoist_get_description;
  iface->get_enabled = gtd_provider_todoist_get_enabled;
  iface->get_icon = gtd_provider_todoist_get_icon;
  iface->get_edit_panel = gtd_provider_todoist_get_edit_panel;
  iface->create_task = gtd_provider_todoist_create_task;
  iface->update_task = gtd_provider_todoist_update_task;
  iface->remove_task = gtd_provider_todoist_remove_task;
  iface->create_task_list = gtd_provider_todoist_create_task_list;
  iface->update_task_list = gtd_provider_todoist_update_task_list;
  iface->remove_task_list = gtd_provider_todoist_remove_task_list;
  iface->get_task_lists = gtd_provider_todoist_get_task_lists;
  iface->get_default_task_list = gtd_provider_todoist_get_default_task_list;
  iface->set_default_task_list = gtd_provider_todoist_set_default_task_list;
}

GtdProviderTodoist*
gtd_provider_todoist_new (GoaObject *account_object)
{

  return g_object_new (GTD_TYPE_PROVIDER_TODOIST,
                       "goa object",account_object,
                       NULL);
}

static void
gtd_provider_todoist_finalize (GObject *object)
{
  GtdProviderTodoist *self = (GtdProviderTodoist *)object;

  g_clear_pointer (&self->lists, g_hash_table_destroy);
  g_clear_object (&self->icon);
  g_clear_pointer (&self->sync_token, g_free);
  g_clear_pointer (&self->description, g_free);

  G_OBJECT_CLASS (gtd_provider_todoist_parent_class)->finalize (object);
}

static void
gtd_provider_todoist_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GtdProvider *provider = GTD_PROVIDER (object);

  switch (prop_id)
    {
    case PROP_DESCRIPTION:
      g_value_set_string (value, gtd_provider_todoist_get_description (provider));
      break;

    case PROP_ENABLED:
      g_value_set_boolean (value, gtd_provider_todoist_get_enabled (provider));
      break;

    case PROP_ICON:
      g_value_set_object (value, gtd_provider_todoist_get_icon (provider));
      break;

    case PROP_ID:
      g_value_set_string (value, gtd_provider_todoist_get_id (provider));
      break;

    case PROP_NAME:
      g_value_set_string (value, gtd_provider_todoist_get_name (provider));
      break;

    case PROP_GOA_OBJECT:
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtd_provider_todoist_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GtdProviderTodoist *self = GTD_PROVIDER_TODOIST (object);

  switch (prop_id)
    {
    case PROP_GOA_OBJECT:
      self->account_object = GOA_OBJECT (g_value_dup_object (value));
      gtd_provider_todoist_set_description (self);
      load_tasks (self);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtd_provider_todoist_class_init (GtdProviderTodoistClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gtd_provider_todoist_finalize;
  object_class->get_property = gtd_provider_todoist_get_property;
  object_class->set_property = gtd_provider_todoist_set_property;

  g_object_class_install_property (object_class,
                                   PROP_GOA_OBJECT,
                                   g_param_spec_object ("goa object",
                                                        "Goa Object",
                                                        "GOA Object around a Todoist Goa Account",
                                                        GOA_TYPE_OBJECT,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_override_property (object_class, PROP_DEFAULT_TASKLIST, "default-task-list");
  g_object_class_override_property (object_class, PROP_DESCRIPTION, "description");
  g_object_class_override_property (object_class, PROP_ENABLED, "enabled");
  g_object_class_override_property (object_class, PROP_ICON, "icon");
  g_object_class_override_property (object_class, PROP_ID, "id");
  g_object_class_override_property (object_class, PROP_NAME, "name");
}

static void
gtd_provider_todoist_init (GtdProviderTodoist *self)
{
  gtd_object_set_ready (GTD_OBJECT (self), TRUE);

  self->lists = g_hash_table_new (g_direct_hash, g_direct_equal);

  /* icon */
  self->icon = G_ICON (g_themed_icon_new_with_default_fallbacks ("computer-symbolic"));
}
