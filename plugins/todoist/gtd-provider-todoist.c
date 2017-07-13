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
#define _XOPEN_SOURCE
#include "gtd-provider-todoist.h"
#include "gtd-plugin-todoist.h"
#include <rest/oauth2-proxy.h>
#include <json-glib/json-glib.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <glib/gi18n.h>

#define TODOIST_URL "https://todoist.com/API/v7/sync"

struct _GtdProviderTodoist
{
  GtdObject          parent;

  GoaObject          *account_object;

  gchar              *sync_token;
  gchar              *access_token;
  gchar              *description;
  GIcon              *icon;

  GHashTable         *lists;
  GHashTable         *tasks;
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
emit_generic_error (const GError *error)
{
  g_warning ("%s: %s: %s",
             G_STRFUNC,
             "Error making a sync call to Todoist",
             error->message);

  gtd_manager_emit_error_message (gtd_manager_get_default (),
                                  _("Error loading Todoist tasks"),
                                  error->message,
                                  NULL,
                                  NULL);
}

static void
emit_access_token_error (void)
{
  g_warning ("%s: %s: %s",
             G_STRFUNC,
             "Error fetching Todoist account access_token",
             "Unable to get access token from gnome-online-accounts");

  gtd_manager_emit_error_message (gtd_manager_get_default (),
                                  _("To Do cannot fetch Todoist account access_token"),
                                  _("Please ensure that Todoist account is correctly configured."),
                                  NULL,
                                  NULL);
}

static gint
get_color_code_index (GdkRGBA *rgba)
{
  guint nearest_color_index;
  guint min_color_diff;
  guint i;

  nearest_color_index = 0;
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
store_access_token (GtdProviderTodoist *self)
{
  GoaOAuth2Based *o_auth2;
  GError *error;

  error = NULL;
  o_auth2 = goa_object_get_oauth2_based (self->account_object);

  if (!goa_oauth2_based_call_get_access_token_sync (o_auth2, &self->access_token, NULL, NULL, &error))
    {
      emit_generic_error (error);
      g_clear_error (&error);
    }
}

static void
parse_array_to_list (GtdProviderTodoist *self,
                     JsonArray          *projects)
{
  GList *lists;
  GList *l;

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

      name = json_object_get_string_member (object, "name");
      color_index = json_object_get_int_member (object, "color");
      id = json_object_get_int_member (object, "id");

      uid = g_strdup_printf ("%u", id);

      gtd_task_list_set_name (list, name);
      gtd_task_list_set_color (list, convert_color_code (color_index));
      gtd_task_list_set_is_removable (list, TRUE);
      gtd_object_set_uid (GTD_OBJECT (list), uid);
      g_hash_table_insert (self->lists, GUINT_TO_POINTER (id), list);
      g_signal_emit_by_name (self, "list-added", list);

      g_free (uid);
    }
}

static GDateTime*
parse_due_date (const gchar *due_date)
{
  GDateTime *dt;
  struct tm due_dt = { 0, };

  if (!strptime (due_date, "%a %d %b %Y %T %z", &due_dt))
    return NULL;

  dt = g_date_time_new_utc (due_dt.tm_year + 1900,
                            due_dt.tm_mon + 1,
                            due_dt.tm_mday,
                            due_dt.tm_hour,
                            due_dt.tm_min,
                            due_dt.tm_sec);

  return dt;
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
      GtdTask *parent_task;
      const gchar *title;
      const gchar *due_date;
      gchar *uid;
      guint32 id;
      guint32 project_id;
      guint32 parent_id;
      gint priority;
      guint is_complete;

      component = e_cal_component_new ();
      e_cal_component_set_new_vtype (component, E_CAL_COMPONENT_TODO);
      e_cal_component_set_uid (component, e_cal_component_gen_uid ());

      task = gtd_task_new (component);

      object = json_node_get_object (l->data);

      title = json_object_get_string_member (object, "content");
      priority = json_object_get_int_member (object, "priority");
      id = json_object_get_int_member (object, "id");
      project_id = json_object_get_int_member (object, "project_id");
      is_complete = json_object_get_int_member (object, "checked");
      due_date = json_object_get_string_member (object, "due_date_utc");

      list = g_hash_table_lookup (self->lists, GUINT_TO_POINTER (project_id));
      uid = g_strdup_printf ("%u", id);
      gtd_object_set_uid (GTD_OBJECT (task), uid);
      gtd_task_set_title (task, title);
      gtd_task_set_list (task, list);
      gtd_task_set_priority (task, priority);
      gtd_task_set_complete (task, is_complete);

      if (!json_object_get_null_member (object, "parent_id"))
        {
          parent_id = json_object_get_int_member (object, "parent_id");
          parent_task = g_hash_table_lookup (self->tasks, GUINT_TO_POINTER (parent_id));
          gtd_task_add_subtask (parent_task, task);
        }

      if (due_date)
        gtd_task_set_due_date (task, parse_due_date (due_date));

      g_hash_table_insert (self->tasks, GUINT_TO_POINTER (id), task);
      gtd_task_list_save_task (list, task);

      g_free (uid);
    }
}

static void
load_tasks (GtdProviderTodoist *self,
            JsonObject         *object)
{
  JsonArray *projects;
  JsonArray *items;

  projects = json_object_get_array_member (object, "projects");
  items = json_object_get_array_member (object, "items");

  parse_array_to_list (self, projects);
  parse_array_to_task (self, items);
}

static gboolean
check_post_response_for_errors (RestProxyCall *call,
                                JsonParser    *parser,
                                const GError  *error)
{
  GError *parse_error;
  const gchar *payload;
  guint status_code;
  gsize payload_length;

  status_code = rest_proxy_call_get_status_code (call);

  if (error)
    {
      emit_generic_error (error);
      return TRUE;
    }

  if (status_code != 200)
    {
      gchar *error_message;

      error_message = g_strdup_printf (_("Bad status code (%d) received. Please check your connection."), status_code);

      gtd_manager_emit_error_message (gtd_manager_get_default (),
                                      _("Error loading Todoist tasks"),
                                      error_message,
                                      NULL,
                                      NULL);
      g_free (error_message);
      return TRUE;
    }

  payload = rest_proxy_call_get_payload (call);
  payload_length = rest_proxy_call_get_payload_length (call);

  if (!json_parser_load_from_data (parser, payload, payload_length, &parse_error))
    {
      emit_generic_error (parse_error);
      g_clear_error (&parse_error);
      return TRUE;
    }

  return FALSE;
}

static void
post (JsonObject                 *params,
      RestProxyCallAsyncCallback  callback,
      gpointer                    user_data)
{
  RestProxy *proxy;
  RestProxyCall *call;
  GList *param;
  GList *l;
  GError *error;

  error = NULL;
  proxy = rest_proxy_new (TODOIST_URL, FALSE);
  call = rest_proxy_new_call (proxy);
  param = json_object_get_members (params);

  rest_proxy_call_set_method (call, "POST");
  rest_proxy_call_add_header (call,
                              "content-type",
                              "application/x-www-form-urlencoded");

  for (l = param; l != NULL; l = l->next)
    {
      JsonNode *node;
      const gchar *value;

      node = json_object_get_member (params, l->data);
      value = json_node_get_string (node);
      rest_proxy_call_add_param (call, l->data, value);
    }

  if (!rest_proxy_call_async (call,
                              callback,
                              NULL,
                              user_data,
                              &error))
    {
      emit_generic_error (error);
      g_clear_error (&error);
      goto out;
    }

out:
  g_object_unref (proxy);
  g_object_unref (call);
  g_list_free (param);
}

static void
synchronize_call_cb (RestProxyCall      *call,
                     const GError       *error,
                     GObject            *weak_object,
                     GtdProviderTodoist *self)
{
  JsonObject *object;
  JsonParser *parser;

  parser = json_parser_new ();

  if (check_post_response_for_errors (call, parser, error))
    goto out;

  object = json_node_get_object (json_parser_get_root (parser));

  if (json_object_has_member (object, "sync_token"))
    self->sync_token = g_strdup (json_object_get_string_member (object, "sync_token"));

  load_tasks (self, object);

out:
  g_object_unref (parser);
}

static void
post_generic_cb (RestProxyCall      *call,
                 const GError       *error,
                 GObject            *weak_object,
                 GtdProviderTodoist *self)
{
  JsonObject *object;
  JsonParser *parser;

  parser = json_parser_new ();

  if (check_post_response_for_errors (call, parser, error))
    goto out;

  object = json_node_get_object (json_parser_get_root (parser));

  if (json_object_has_member (object, "sync_token"))
    self->sync_token = g_strdup (json_object_get_string_member (object, "sync_token"));

out:
  g_object_unref (parser);
}

static void
synchronize_call (GtdProviderTodoist *self)
{
  JsonObject *params;

  if (!self->access_token)
    {
      emit_access_token_error ();
      return;
    }

  params = json_object_new ();

  json_object_set_string_member (params, "token", self->access_token);
  json_object_set_string_member (params, "sync_token", self->sync_token);
  json_object_set_string_member (params, "resource_types", "[\"all\"]");

  post (params, (RestProxyCallAsyncCallback) synchronize_call_cb, self);

  json_object_unref (params);
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
  GtdProviderTodoist *self;
  JsonObject *params;
  GtdTask *parent;
  GDateTime *due_date;
  g_autofree gchar *command;
  g_autofree gchar *command_uuid;
  g_autofree gchar *due_dt;
  const gchar *task_uid;
  const gchar *title;
  const gchar *parent_id;
  gint priority;
  gint indent;
  gint checked;

  self = GTD_PROVIDER_TODOIST (provider);
  due_dt = command = command_uuid = NULL;

  if (!self->access_token)
    {
      emit_access_token_error ();
      return;
    }

  params = json_object_new ();
  task_uid = gtd_object_get_uid (GTD_OBJECT (task));
  title = gtd_task_get_title (task);
  priority = gtd_task_get_priority (task);
  parent = gtd_task_get_parent (task);
  indent = gtd_task_get_depth (task) + 1;
  checked =  gtd_task_get_complete (task);
  parent_id = parent ? gtd_object_get_uid (GTD_OBJECT (parent)) : "null";
  due_date = gtd_task_get_due_date (task);

  if (due_date)
    {
      g_autofree gchar *date_format;

      date_format = g_date_time_format (due_date, "%FT%R");
      due_dt = g_strdup_printf ("\"%s\"", date_format);
    }
  else
    {
      due_dt = g_strdup ("null");
    }

  command_uuid = g_uuid_string_random ();
  command = g_strdup_printf ("[{\"type\": \"item_update\", \"uuid\": \"%s\", "
                             "\"args\": {\"id\": %s, \"content\": \"%s\", "
                             "\"priority\": %d, \"parent_id\": %s, "
                             "\"indent\": %d, \"checked\": %d, "
                             "\"due_date_utc\": %s}}]",
                             command_uuid,
                             task_uid,
                             title,
                             priority,
                             parent_id,
                             indent,
                             checked,
                             due_dt);

  json_object_set_string_member (params, "token", self->access_token);
  json_object_set_string_member (params, "commands", command);

  post (params, (RestProxyCallAsyncCallback) post_generic_cb, self);

  g_clear_pointer (&due_date, g_date_time_unref);
}

static void
gtd_provider_todoist_remove_task (GtdProvider *provider,
                                   GtdTask     *task)
{
  GtdProviderTodoist *self;
  JsonObject *params;
  g_autofree gchar *command;
  g_autofree gchar *command_uuid;
  const gchar *task_uid;

  self = GTD_PROVIDER_TODOIST (provider);
  command = command_uuid = NULL;

  if (!self->access_token)
    {
      emit_access_token_error ();
      return;
    }

  params = json_object_new ();
  task_uid = gtd_object_get_uid (GTD_OBJECT (task));

  command_uuid = g_uuid_string_random ();
  command = g_strdup_printf ("[{\"type\": \"item_delete\", \"uuid\": \"%s\", "
                             "\"args\": {\"ids\": [%s]}}]",
                             command_uuid,
                             task_uid);

  json_object_set_string_member (params, "token", self->access_token);
  json_object_set_string_member (params, "commands", command);

  post (params, (RestProxyCallAsyncCallback) post_generic_cb, self);
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
  GtdProviderTodoist *self;
  JsonObject *params;
  GdkRGBA *list_color;
  g_autofree gchar *command;
  g_autofree gchar *command_uuid;
  const gchar *list_uid;
  const gchar *list_name;
  gint color_index;

  self = GTD_PROVIDER_TODOIST (provider);
  command = command_uuid = NULL;

  if (!self->access_token)
    {
      emit_access_token_error ();
      return;
    }

  params = json_object_new ();
  list_uid = gtd_object_get_uid (GTD_OBJECT (list));
  list_name = gtd_task_list_get_name (list);
  list_color = gtd_task_list_get_color (list);
  color_index = get_color_code_index (list_color);

  command_uuid = g_uuid_string_random ();
  command = g_strdup_printf ("[{\"type\": \"project_update\", \"uuid\": \"%s\", "
                             "\"args\": {\"id\": %s, \"name\": \"%s\", \"color\": %d}}]",
                             command_uuid,
                             list_uid,
                             list_name,
                             color_index);

  json_object_set_string_member (params, "token", self->access_token);
  json_object_set_string_member (params, "commands", command);

  post (params, (RestProxyCallAsyncCallback) post_generic_cb, self);

  gdk_rgba_free (list_color);
}

static void
gtd_provider_todoist_remove_task_list (GtdProvider *provider,
                                       GtdTaskList *list)
{
  GtdProviderTodoist *self;
  JsonObject *params;
  g_autofree gchar *command;
  g_autofree gchar *command_uuid;
  const gchar *list_uid;

  self = GTD_PROVIDER_TODOIST (provider);
  command = command_uuid = NULL;

  if (!self->access_token)
    {
      emit_access_token_error ();
      return;
    }

  params = json_object_new ();
  list_uid = gtd_object_get_uid (GTD_OBJECT (list));

  command_uuid = g_uuid_string_random ();
  command = g_strdup_printf ("[{\"type\": \"project_delete\", \"uuid\": \"%s\", "
                             "\"args\": {\"ids\": [%s]}}]",
                             command_uuid,
                             list_uid);

  json_object_set_string_member (params, "token", self->access_token);
  json_object_set_string_member (params, "commands", command);

  post (params, (RestProxyCallAsyncCallback) post_generic_cb, self);
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
update_description (GtdProviderTodoist *self)
{
  GoaAccount *account;
  const gchar *identity;

  account = goa_object_get_account (self->account_object);
  identity = goa_account_get_identity (account);
  self->description = g_strdup_printf (_("Todoist: %s"), identity);

  g_object_unref (account);
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
  g_clear_pointer (&self->tasks, g_hash_table_destroy);
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
      update_description (self);
      store_access_token (self);
      if (self->access_token)
        synchronize_call (self);
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
  self->tasks = g_hash_table_new (g_direct_hash, g_direct_equal);
  self->sync_token = g_strdup ("*");

  /* icon */
  self->icon = G_ICON (g_themed_icon_new_with_default_fallbacks ("computer-symbolic"));
}
