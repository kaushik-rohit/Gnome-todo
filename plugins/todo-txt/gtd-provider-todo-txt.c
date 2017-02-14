/* gtd-provider-todo-txt.c
 *
 * Copyright (C) 2016 Rohit Kaushik <kaushikrohit325@gmail.com>
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

#include "gtd-provider-todo-txt.h"
#include "gtd-plugin-todo-txt.h"
#include "gtd-todo-txt-parser.h"
#include "gtd-task-list.h"
#include "gtd-task.h"
#include <string.h>
#include <stdlib.h>

#include <glib/gi18n.h>


struct _GtdProviderTodoTxt
{
  GtdObject          parent;

  GIcon              *icon;

  GHashTable         *lists;
  GHashTable         *root_tasks;

  GFileMonitor       *monitor;
  GFile              *source_file;

  gchar              *source;
  GList              *tasklists;

  gint                no_of_lines;
};

static void          gtd_provider_iface_init                     (GtdProviderInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GtdProviderTodoTxt, gtd_provider_todo_txt, GTD_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GTD_TYPE_PROVIDER,
                                                gtd_provider_iface_init))

enum {
  PROP_0,
  PROP_ENABLED,
  PROP_ICON,
  PROP_ID,
  PROP_NAME,
  PROP_DESCRIPTION,
  PROP_SOURCE,
  LAST_PROP
};

/*
 * GtdProviderInterface implementation
 */
static const gchar*
gtd_provider_todo_txt_get_id (GtdProvider *provider)
{
  return "todo-txt";
}

static const gchar*
gtd_provider_todo_txt_get_name (GtdProvider *provider)
{
  return _("Todo.txt");
}

static const gchar*
gtd_provider_todo_txt_get_description (GtdProvider *provider)
{
  return _("On the Todo.txt file");
}


static gboolean
gtd_provider_todo_txt_get_enabled (GtdProvider *provider)
{
  return TRUE;
}

static GIcon*
gtd_provider_todo_txt_get_icon (GtdProvider *provider)
{
  GtdProviderTodoTxt *self;

  self = GTD_PROVIDER_TODO_TXT (provider);

  return self->icon;
}

static const GtkWidget*
gtd_provider_todo_txt_get_edit_panel (GtdProvider *provider)
{
  return NULL;
}

static GtdTask*
gtd_provider_todo_txt_create_new_task (void)
{
  ECalComponent *component;
  GtdTask *task;

  component = e_cal_component_new ();

  e_cal_component_set_new_vtype (component, E_CAL_COMPONENT_TODO);
  e_cal_component_set_uid (component, e_cal_component_gen_uid ());

  task = gtd_task_new (component);

  return task;
}

static void
gtd_provider_todo_txt_load_task (TaskData           *td,
                                 GtdProviderTodoTxt *self,
                                 gint                line_number)
{
  GtdTaskList *task_list = NULL;

  gchar *task_list_name;
  gchar *root_task_name;
  gchar *title;

  GDateTime    *due_date;

  gboolean  is_subtask;
  gboolean  is_task_completed;
  gint      priority;

  task_list_name = gtd_todo_txt_parser_task_data_get_task_list_name (td);
  root_task_name = gtd_todo_txt_parser_task_data_get_root_task_name (td);
  title = gtd_todo_txt_parser_task_data_get_title (td);
  due_date = gtd_todo_txt_parser_task_data_get_due_date (td);
  is_subtask = gtd_todo_txt_parser_task_data_is_subtask (td);
  is_task_completed = gtd_todo_txt_parser_task_data_is_task_completed (td);
  priority = gtd_todo_txt_parser_task_data_get_priority (td);

  if (is_subtask)
    {
      GtdTask *root_task;
      GtdTask *sub_task;

      if (g_hash_table_contains (self->lists, task_list_name))
        task_list = g_hash_table_lookup (self->lists, task_list_name);
      else
        {
          task_list = gtd_task_list_new (GTD_PROVIDER (self));
          gtd_task_list_set_is_removable (task_list, TRUE);

          gtd_task_list_set_name (task_list, task_list_name);
          self->tasklists = g_list_append (self->tasklists,
                                           task_list);

          g_object_set_data (G_OBJECT (task_list), "line", task_list_name);

          g_signal_emit_by_name (self, "list-added", task_list);
          g_hash_table_insert (self->lists, task_list_name, task_list);
        }

      if (g_hash_table_contains (self->root_tasks, root_task_name))
        {
          root_task = g_hash_table_lookup (self->root_tasks, root_task_name);
        }
      else
        {
          root_task = gtd_provider_todo_txt_create_new_task ();

          gtd_task_set_title (root_task, root_task_name);
          gtd_task_set_list  (root_task, task_list);
          gtd_task_list_save_task (task_list, root_task);

          g_hash_table_insert (self->root_tasks, root_task_name, root_task);
        }

      sub_task = gtd_provider_todo_txt_create_new_task ();

      gtd_task_set_title (sub_task, title);
      gtd_task_set_list (sub_task, task_list);
      gtd_task_set_priority (sub_task, priority);
      gtd_task_set_complete (sub_task, is_task_completed);
      gtd_task_set_due_date (sub_task, due_date);

      g_object_set_data (G_OBJECT (sub_task), "line", GINT_TO_POINTER (line_number));

      gtd_task_add_subtask (root_task, sub_task);
      gtd_task_list_save_task (task_list, sub_task);
    }
  else
    {
      GtdTask *task;

      if (g_hash_table_contains (self->lists, task_list_name))
        {
          task_list = g_hash_table_lookup (self->lists, task_list_name);
        }
      else
        {
          task_list = gtd_task_list_new (GTD_PROVIDER (self));
          gtd_task_list_set_is_removable (task_list, TRUE);

          gtd_task_list_set_name (task_list, task_list_name);
          self->tasklists = g_list_append (self->tasklists,
                                           task_list);

          g_object_set_data (G_OBJECT (task_list), "line", task_list_name);

          g_signal_emit_by_name (self, "list-added", task_list);
          g_hash_table_insert (self->lists, task_list_name, task_list);
        }
      task = gtd_provider_todo_txt_create_new_task ();

      gtd_task_set_title (task, title);
      gtd_task_set_list (task, task_list);
      gtd_task_set_priority (task, priority);
      gtd_task_set_complete (task, is_task_completed);
      gtd_task_set_due_date (task, due_date);

      g_object_set_data (G_OBJECT (task), "line", GINT_TO_POINTER (line_number));

      gtd_task_list_save_task (task_list, task);
      g_hash_table_insert (self->root_tasks, title, task);
    }
}

static void
gtd_provider_todo_txt_create_empty_list (GtdProviderTodoTxt *self,
                                         gchar              *line)
{
  GtdTaskList *task_list;

  if (g_hash_table_contains (self->lists, line))
    return;

  task_list = gtd_task_list_new (GTD_PROVIDER (self));
  gtd_task_list_set_is_removable (task_list, TRUE);

  gtd_task_list_set_name (task_list, line);
  self->tasklists = g_list_append (self->tasklists,
                                           task_list);
  g_object_set_data (G_OBJECT (task_list), "line", line);

  g_signal_emit_by_name (self, "list-added", task_list);
  g_hash_table_insert (self->lists, line, task_list);
}

static void
gtd_provider_todo_txt_load_source (GtdProviderTodoTxt *self)
{
  GFileInputStream *readstream;
  GError *error = NULL;
  gint line_number = 0;

  g_return_if_fail (G_IS_FILE (self->source_file));

  readstream = g_file_read (self->source_file,
                            NULL,
                            &error);

  if (!error)
    {
      GDataInputStream *reader;
      gchar *line_read;
      GError *line_read_error = NULL;
      TaskData *td = NULL;

      reader = g_data_input_stream_new (G_INPUT_STREAM (readstream));
      self->no_of_lines = 0;

      while (!line_read_error)
        {
          GList *tokens = NULL;
          gboolean valid;
          line_read = g_data_input_stream_read_line (reader,
                                                     NULL,
                                                     NULL,
                                                     &line_read_error);

          if (!line_read_error)
            {
              if (!line_read)
                break;

              line_number++;
              self->no_of_lines++;

              tokens = gtd_todo_txt_parser_tokenize (g_strdup(line_read));
              valid = gtd_todo_txt_parser_validate_token_format (tokens);

              if (valid)
                {
                  td = gtd_todo_txt_parser_parse_tokens (tokens);

                  if (strcmp (gtd_todo_txt_parser_task_data_get_task_list_name (td), &(line_read[1])) == 0)
                    gtd_provider_todo_txt_create_empty_list (self, g_strdup (&(line_read[1])));
                  else
                    gtd_provider_todo_txt_load_task (td, self, line_number);
                }
            }
          else
            {
              g_warning ("%s: %s: %s",
                         G_STRFUNC,
                         _("Error reading line from todo.txt"),
                         line_read_error->message);

              gtd_manager_emit_error_message (gtd_manager_get_default (),
                                              _("Error reading line from Todo.txt"),
                                              line_read_error->message);
              g_error_free (line_read_error);

              return;
            }

          g_list_free_full (tokens, g_free);
          g_free (line_read);
        }
    }
  else
    {
      g_warning ("%s: %s: %s",
                 G_STRFUNC,
                 _("Error opening todo.txt file"),
                 error->message);

      gtd_manager_emit_error_message (gtd_manager_get_default (),
                                      _("Error opening todo.txt file"),
                                      error->message);
      g_error_free (error);
      return;
    }
}

static void
gtd_provider_todo_txt_create_task (GtdProvider *provider,
                                   GtdTask     *task)
{
  GtdProviderTodoTxt *self;
  GFileOutputStream *write_stream;
  GtdTaskList *list;
  GError *error;
  const gchar *list_name;
  const gchar *task_description;
  gchar *task_line;

  self = GTD_PROVIDER_TODO_TXT (provider);
  error = NULL;

  g_return_if_fail (G_IS_FILE (self->source_file));

  if (self->monitor)
    g_signal_handlers_block_by_func(self->monitor, gtd_plugin_todo_txt_monitor_source, self);

  list = gtd_task_get_list (task);
  list_name = gtd_task_list_get_name (list);
  task_description = gtd_task_get_title (task);

  task_line = g_strconcat (task_description, " ", "@", list_name, "\n", NULL);

  write_stream = g_file_append_to (self->source_file,
                                  G_FILE_CREATE_REPLACE_DESTINATION,
                                  NULL,
                                  &error);

  if(!error)
    {
      GDataOutputStream *writer;
      GError *write_error = NULL;

      writer = g_data_output_stream_new (G_OUTPUT_STREAM (write_stream));

      g_data_output_stream_put_string (writer,
                                       task_line,
                                       NULL,
                                       &write_error);

      if(write_error)
        {
          g_warning ("%s: %s: %s",
                     G_STRFUNC,
                     _("Error adding task to Todo.txt"),
                     write_error->message);

          gtd_manager_emit_error_message (gtd_manager_get_default (),
                                          _("Error adding task to Todo.txt"),
                                          write_error->message);
          g_error_free (write_error);
          return;
        }
      else
        {
          g_hash_table_insert (self->root_tasks,
                               (gpointer) task_description,
                               task);
          self->no_of_lines++;
          g_object_set_data (G_OBJECT (task), "line", GINT_TO_POINTER (self->no_of_lines));
        }

      g_output_stream_close (G_OUTPUT_STREAM (writer),
                             NULL,
                             NULL);
    }
  else
    {
      g_warning ("%s: %s: %s",
                 G_STRFUNC,
                 _("Error opening Todo.txt"),
                 error->message);

      gtd_manager_emit_error_message (gtd_manager_get_default (),
                                      _("Error opening Todo.txt"),
                                      error->message);
      g_error_free (error);
    }
  if (self->monitor)
    g_signal_handlers_unblock_by_func(self->monitor, gtd_plugin_todo_txt_monitor_source, self);

  g_free (task_line);
}

static void
gtd_provider_todo_txt_update_task (GtdProvider *provider,
                                   GtdTask     *task)
{
  GtdProviderTodoTxt *self;
  GFileInputStream *readstream;
  GFileOutputStream *outstream;
  GDataOutputStream *writer;
  GError *error;
  GError *write_error;
  gint line_to_update;
  gint line_number;

  self = GTD_PROVIDER_TODO_TXT (provider);
  line_number = 0;
  error = write_error = NULL;

  if (self->monitor)
    g_signal_handlers_block_by_func(self->monitor, gtd_plugin_todo_txt_monitor_source, self);

  line_to_update = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (task), "line"));

  g_return_if_fail (G_IS_FILE (self->source_file));

  readstream = g_file_read (self->source_file,
                            NULL,
                            &error);
  outstream = g_file_replace (self->source_file,
                              NULL,
                              TRUE,
                              G_FILE_CREATE_NONE,
                              NULL,
                              &error);

  if (!error)
    {
      GDataInputStream *reader;
      GError *line_read_error = NULL;
      gchar *line_read;

      reader = g_data_input_stream_new (G_INPUT_STREAM (readstream));
      writer = g_data_output_stream_new (G_OUTPUT_STREAM (outstream));

      while (!line_read_error)
        {
          gboolean valid;
          GList *tokens = NULL;

          line_read = g_data_input_stream_read_line (reader,
                                                     NULL,
                                                     NULL,
                                                     &line_read_error);

          if (!line_read_error)
            {
              if (!line_read)
                break;

              line_number++;

              tokens = gtd_todo_txt_parser_tokenize (g_strdup (line_read));
              valid = gtd_todo_txt_parser_validate_token_format (tokens);

              if (valid &&
                  line_number == line_to_update)
                {
                  GList *update_tokens;
                  GList *it;

                  update_tokens = gtd_todo_txt_parser_get_task_line (task);
                  it = NULL;
                  for (it = update_tokens; it != NULL; it = it->next)
                    {
                      g_data_output_stream_put_string (writer,
                                                       it->data,
                                                       NULL,
                                                       &write_error);
                      if (it->next == NULL)
                        {
                          g_data_output_stream_put_string (writer,
                                                           "\n",
                                                           NULL,
                                                           &write_error);
                        }
                      else
                        g_data_output_stream_put_string (writer,
                                                         " ",
                                                         NULL,
                                                         &write_error);
                    }

                  g_list_free_full (update_tokens, g_free);
                }


              else
                {
                  line_read = strcat (line_read, "\n");

                  g_data_output_stream_put_string (writer,
                                                   line_read,
                                                   NULL,
                                                   &write_error);
                }
            }
          else
            {
              g_warning ("%s: %s: %s",
                         G_STRFUNC,
                         _("Error reading tasks from Todo.txt"),
                         line_read_error->message);

              gtd_manager_emit_error_message (gtd_manager_get_default (),
                                              _("Error reading tasks from Todo.txt"),
                                              line_read_error->message);
              g_error_free (line_read_error);
            }

          g_list_free_full (tokens, g_free);
        }

      g_output_stream_close (G_OUTPUT_STREAM (writer),
                             NULL,
                             NULL);
    }
  else
    {
      g_warning ("%s: %s: %s",
                 G_STRFUNC,
                 _("Error opening Todo.txt"),
                 error->message);

      gtd_manager_emit_error_message (gtd_manager_get_default (),
                                      _("Error opening Todo.txt"),
                                      error->message);
      g_error_free (error);
    }
  if (self->monitor)
    g_signal_handlers_unblock_by_func(self->monitor, gtd_plugin_todo_txt_monitor_source, self);
}

static void
gtd_provider_todo_txt_remove_task (GtdProvider *provider,
                                   GtdTask     *task)
{
  GtdProviderTodoTxt *self;
  GFileInputStream *readstream;
  GFileOutputStream *outstream;
  GDataOutputStream *writer;
  GError *error;
  GError *write_error;
  gint line_number;
  gint line_number_to_remove;

  self = GTD_PROVIDER_TODO_TXT (provider);
  error = write_error = NULL;
  line_number = 0;

  g_return_if_fail (G_IS_FILE (self->source_file));

  if (self->monitor)
    g_signal_handlers_block_by_func(self->monitor, gtd_plugin_todo_txt_monitor_source, self);

  line_number_to_remove = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (task), "line"));
  readstream = g_file_read (self->source_file,
                            NULL,
                            &error);
  outstream = g_file_replace (self->source_file,
                              NULL,
                              TRUE,
                              G_FILE_CREATE_NONE,
                              NULL,
                              &error);

  if (!error)
    {
      GDataInputStream *reader;
      GError *line_read_error = NULL;
      gchar *line_read;
      gboolean skip;

      reader = g_data_input_stream_new (G_INPUT_STREAM (readstream));
      writer = g_data_output_stream_new (G_OUTPUT_STREAM (outstream));

      while (!line_read_error)
        {
          TaskData *td;
          gboolean valid;
          GList *tokens = NULL;

          line_read = g_data_input_stream_read_line (reader,
                                                     NULL,
                                                     NULL,
                                                     &line_read_error);

          skip = FALSE;

          if (!line_read_error)
            {
              if (!line_read)
                break;
              line_number++;

              tokens = gtd_todo_txt_parser_tokenize (g_strdup(line_read));
              valid = gtd_todo_txt_parser_validate_token_format (tokens);

              if (valid)
                td = gtd_todo_txt_parser_parse_tokens (tokens);
              else
                td = NULL;

              if (line_number == line_number_to_remove)
                {
                  skip = TRUE;
                  self->no_of_lines--;
                }

              if (!skip)
                {
                  line_read = strcat (line_read, "\n");

                  g_data_output_stream_put_string (writer,
                                                   line_read,
                                                   NULL,
                                                  &write_error);
                }
            }
          else
            {
              g_warning ("%s: %s: %s",
                         G_STRFUNC,
                         _("Error reading tasks from Todo.txt"),
                         line_read_error->message);

              gtd_manager_emit_error_message (gtd_manager_get_default (),
                                              _("Error reading tasks from Todo.txt"),
                                              line_read_error->message);
              g_error_free (line_read_error);

              return;
            }
          g_free (td);
        }

      g_output_stream_close (G_OUTPUT_STREAM (writer),
                             NULL,
                             NULL);
    }
  else
    {
      g_warning ("%s: %s: %s",
                 G_STRFUNC,
                 _("Error opening Todo.txt"),
                 error->message);

      gtd_manager_emit_error_message (gtd_manager_get_default (),
                                      _("Error opening Todo.txt"),
                                      error->message);
      g_error_free (error);
    }
  if (self->monitor)
    g_signal_handlers_unblock_by_func(self->monitor, gtd_plugin_todo_txt_monitor_source, self);
}

static void
gtd_provider_todo_txt_create_task_list (GtdProvider *provider,
                                        GtdTaskList *list)
{
  GtdProviderTodoTxt *self;
  GFileOutputStream  *write_stream;
  GError *error;
  const gchar *name;

  self = GTD_PROVIDER_TODO_TXT (provider);
  error = NULL;

  g_return_if_fail (G_IS_FILE (self->source_file));

  if (self->monitor)
    g_signal_handlers_block_by_func (self->monitor, gtd_plugin_todo_txt_monitor_source, self);

  write_stream = g_file_append_to (self->source_file,
                                  G_FILE_CREATE_REPLACE_DESTINATION,
                                  NULL,
                                  &error);

  if(!error)
    {
      GDataOutputStream *writer;
      GError *write_error = NULL;
      gchar *put;

      name = gtd_task_list_get_name (list);
      writer = g_data_output_stream_new (G_OUTPUT_STREAM (write_stream));

      put = g_strconcat ("@", name, "\n", NULL);

      g_data_output_stream_put_string (writer,
                                       put,
                                       NULL,
                                       &write_error);

      if(write_error)
        {
          g_warning ("%s: %s: %s",
                     G_STRFUNC,
                     _("Error creating Todo.txt list"),
                     write_error->message);

          gtd_manager_emit_error_message (gtd_manager_get_default (),
                                          _("Error creating todo.txt list"),
                                          write_error->message);
          g_error_free (write_error);
          return;
        }
      else
        {
          self->tasklists = g_list_append (self->tasklists,
                                           list);
          g_hash_table_insert (self->lists, (gpointer) name, list);
          g_signal_emit_by_name (self, "list-added", list);
          self->no_of_lines++;
        }

      g_free (put);
    }
  else
    {
      g_warning ("%s: %s: %s",
                 G_STRFUNC,
                 _("Error opening Todo.txt"),
                 error->message);

      gtd_manager_emit_error_message (gtd_manager_get_default (),
                                      _("Error opening Todo.txt"),
                                      error->message);
      g_error_free (error);
    }
  if (self->monitor)
    g_signal_handlers_unblock_by_func(self->monitor, gtd_plugin_todo_txt_monitor_source, self);
}

static void
gtd_provider_todo_txt_update_task_list (GtdProvider *provider,
                                        GtdTaskList *list)
{
  GtdProviderTodoTxt *self;
  GFileInputStream *readstream;
  GFileOutputStream *outstream;
  GDataOutputStream *writer;
  GError *error;
  GError *write_error;
  const gchar *current_list_name;
  gchar *stored_list_name;

  self = GTD_PROVIDER_TODO_TXT (provider);
  error = write_error = NULL;

  g_return_if_fail (G_IS_FILE (self->source_file));

  if (self->monitor)
    g_signal_handlers_block_by_func(self->monitor, gtd_plugin_todo_txt_monitor_source, self);

  stored_list_name = g_object_get_data (G_OBJECT (list), "line");
  current_list_name = gtd_task_list_get_name (list);

  readstream = g_file_read (self->source_file,
                            NULL,
                            &error);
  outstream = g_file_replace (self->source_file,
                              NULL,
                              TRUE,
                              G_FILE_CREATE_NONE,
                              NULL,
                              &error);

  if (!error)
    {
      GDataInputStream *reader;
      GError *line_read_error = NULL;
      char *line_read;

      reader = g_data_input_stream_new (G_INPUT_STREAM (readstream));
      writer = g_data_output_stream_new (G_OUTPUT_STREAM (outstream));

      while (!line_read_error)
        {
          gboolean valid;
          GList *tokens;
          TaskData *td = NULL;

          tokens = NULL;
          line_read = g_data_input_stream_read_line (reader,
                                                     NULL,
                                                     NULL,
                                                     &line_read_error);

          if (!line_read_error)
            {
              if (!line_read)
                break;

              tokens = gtd_todo_txt_parser_tokenize (g_strdup(line_read));
              valid = gtd_todo_txt_parser_validate_token_format (tokens);

              if (valid)
                td = gtd_todo_txt_parser_parse_tokens (tokens);

              if (valid &&
                  strcmp (stored_list_name, current_list_name)&&
                  !(strcmp (gtd_todo_txt_parser_task_data_get_task_list_name (td), stored_list_name)))
                {
                  GList *update_tokens;
                  GList *it;

                  update_tokens = gtd_todo_txt_parser_get_list_updated_token (list, g_strdup (line_read));
                  it = NULL;

                  for (it = update_tokens; it != NULL; it = it->next)
                    {
                      g_data_output_stream_put_string (writer,
                                                       it->data,
                                                       NULL,
                                                       &write_error);
                      if (it->next == NULL)
                        {
                          g_data_output_stream_put_string (writer,
                                                           "\n",
                                                           NULL,
                                                           &write_error);
                        }
                      else
                        g_data_output_stream_put_string (writer,
                                                         " ",
                                                         NULL,
                                                         &write_error);
                    }
                  g_list_free_full (update_tokens, g_free);
                }


              else
                {
                  line_read = strcat (line_read, "\n");

                  g_data_output_stream_put_string (writer,
                                                   line_read,
                                                   NULL,
                                                   &write_error);
                }
            }
          else
            {
              g_warning ("%s: %s: %s",
                         G_STRFUNC,
                         _("Error while reading tasks from Todo.txt"),
                         line_read_error->message);

              gtd_manager_emit_error_message (gtd_manager_get_default (),
                                             _("Error while reading tasks from Todo.txt"),
                                               line_read_error->message);
              g_error_free (line_read_error);
            }

          g_list_free_full (tokens, g_free);
        }

      g_output_stream_close (G_OUTPUT_STREAM (writer),
                             NULL,
                             NULL);
      g_input_stream_close (G_INPUT_STREAM (reader),
                            NULL,
                            NULL);
    }
  else
    {
      g_warning ("%s: %s: %s",
                 G_STRFUNC,
                 _("Error opening Todo.txt"),
                 error->message);

      gtd_manager_emit_error_message (gtd_manager_get_default (),
                                      _("Error opening Todo.txt"),
                                      error->message);
      g_error_free (error);
    }
  g_input_stream_close (G_INPUT_STREAM (readstream),
                        NULL,
                        NULL);
  g_output_stream_close (G_OUTPUT_STREAM (outstream),
                         NULL,
                         NULL);
  if (self->monitor)
    g_signal_handlers_unblock_by_func(self->monitor, gtd_plugin_todo_txt_monitor_source, self);
}

static void
gtd_provider_todo_txt_remove_task_list (GtdProvider *provider,
                                        GtdTaskList *list)
{

  GtdProviderTodoTxt *self;
  GFileOutputStream *outstream;
  GDataOutputStream *writer;
  GFileInputStream *readstream;
  GError *write_error;
  GError *error;
  const gchar  *list_name;

  self = GTD_PROVIDER_TODO_TXT (provider);
  error = write_error = NULL;
  list_name = gtd_task_list_get_name (list);

  g_return_if_fail (G_IS_FILE (self->source_file));

  if (self->monitor)
    g_signal_handlers_block_by_func(self->monitor, gtd_plugin_todo_txt_monitor_source, self);

  readstream = g_file_read (self->source_file,
                            NULL,
                            &error);
  outstream = g_file_replace (self->source_file,
                              NULL,
                              TRUE,
                              G_FILE_CREATE_NONE,
                              NULL,
                              &error);

  if (!error)
    {
      GDataInputStream *reader;
      GError *line_read_error = NULL;
      char *line;
      gboolean skip;

      reader = g_data_input_stream_new (G_INPUT_STREAM (readstream));
      writer = g_data_output_stream_new (G_OUTPUT_STREAM (outstream));

      while (!line_read_error)
        {
          TaskData *td = NULL;
          gboolean valid;
          GList *tokens = NULL;

          line = g_data_input_stream_read_line (reader,
                                                NULL,
                                                NULL,
                                                &line_read_error);

          skip = FALSE;

          if (!line_read_error)
            {
              if (!line)
                break;

              tokens = gtd_todo_txt_parser_tokenize (g_strdup(line));
              valid = gtd_todo_txt_parser_validate_token_format (tokens);

              if (valid)
                td = gtd_todo_txt_parser_parse_tokens (tokens);

              if (valid && strcmp (gtd_todo_txt_parser_task_data_get_task_list_name (td), list_name) == 0)
                {
                  self->no_of_lines--;
                  skip = TRUE;
                }

              if (!skip)
                {
                  line = strcat (line, "\n");

                  g_data_output_stream_put_string (writer,
                                                   line,
                                                   NULL,
                                                   &write_error);
                }
            }
          else
            {
              g_warning ("%s: %s: %s",
                         G_STRFUNC,
                         _("Error reading task-lists from Todo.txt"),
                         line_read_error->message);

              gtd_manager_emit_error_message (gtd_manager_get_default (),
                                              _("Error reading task-lists from Todo.txt"),
                                              line_read_error->message);
              g_error_free (line_read_error);
            }
          g_free (td);
        }
      g_output_stream_close (G_OUTPUT_STREAM (writer),
                             NULL,
                             NULL);
    }
  else
    {
      g_warning ("%s: %s: %s",
                 G_STRFUNC,
                 _("Error opening todo.txt file"),
                 error->message);

      gtd_manager_emit_error_message (gtd_manager_get_default (),
                                      _("Error opening Todo.txt"),
                                      error->message);
      g_error_free (error);
    }
  if (self->monitor)
    g_signal_handlers_unblock_by_func(self->monitor, gtd_plugin_todo_txt_monitor_source, self);
}

static GList*
gtd_provider_todo_txt_get_task_lists (GtdProvider *provider)
{
  GtdProviderTodoTxt *self;

  self = GTD_PROVIDER_TODO_TXT (provider);

  return self->tasklists;
}

static GtdTaskList*
gtd_provider_todo_txt_get_default_task_list (GtdProvider *provider)
{
  return NULL;
}

static void
gtd_provider_iface_init (GtdProviderInterface *iface)
{
  iface->get_id = gtd_provider_todo_txt_get_id;
  iface->get_name = gtd_provider_todo_txt_get_name;
  iface->get_description = gtd_provider_todo_txt_get_description;
  iface->get_enabled = gtd_provider_todo_txt_get_enabled;
  iface->get_icon = gtd_provider_todo_txt_get_icon;
  iface->get_edit_panel = gtd_provider_todo_txt_get_edit_panel;
  iface->create_task = gtd_provider_todo_txt_create_task;
  iface->update_task = gtd_provider_todo_txt_update_task;
  iface->remove_task = gtd_provider_todo_txt_remove_task;
  iface->create_task_list = gtd_provider_todo_txt_create_task_list;
  iface->update_task_list = gtd_provider_todo_txt_update_task_list;
  iface->remove_task_list = gtd_provider_todo_txt_remove_task_list;
  iface->get_task_lists = gtd_provider_todo_txt_get_task_lists;
  iface->get_default_task_list = gtd_provider_todo_txt_get_default_task_list;
}

GtdProviderTodoTxt*
gtd_provider_todo_txt_new (gchar *source)
{

  return g_object_new (GTD_TYPE_PROVIDER_TODO_TXT,
                       "source", source,
                       NULL);
}

static void
gtd_provider_todo_txt_finalize (GObject *object)
{
  GtdProviderTodoTxt *self = (GtdProviderTodoTxt *)object;

  g_clear_pointer (&self->lists, g_hash_table_destroy);
  g_clear_pointer (&self->root_tasks, g_hash_table_destroy);
  g_clear_pointer (&self->tasklists, g_clear_object);
  g_clear_pointer (&self->source_file, g_free);
  g_clear_object (&self->icon);
  g_clear_pointer (&self->source, g_free);

  G_OBJECT_CLASS (gtd_provider_todo_txt_parent_class)->finalize (object);
}

static void
gtd_provider_todo_txt_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GtdProvider *provider = GTD_PROVIDER (object);

  switch (prop_id)
    {
    case PROP_DESCRIPTION:
      g_value_set_string (value, gtd_provider_todo_txt_get_description (provider));
      break;

    case PROP_ENABLED:
      g_value_set_boolean (value, gtd_provider_todo_txt_get_enabled (provider));
      break;

    case PROP_ICON:
      g_value_set_object (value, gtd_provider_todo_txt_get_icon (provider));
      break;

    case PROP_ID:
      g_value_set_string (value, gtd_provider_todo_txt_get_id (provider));
      break;

    case PROP_NAME:
      g_value_set_string (value, gtd_provider_todo_txt_get_name (provider));
      break;

    case PROP_SOURCE:
      g_value_set_string (value, GTD_PROVIDER_TODO_TXT (provider)->source);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtd_provider_todo_txt_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  GtdProviderTodoTxt *self = GTD_PROVIDER_TODO_TXT (object);
  switch (prop_id)
    {
    case PROP_SOURCE:
      self->source = g_value_dup_string (value);
      self->source_file = g_file_new_for_uri (self->source);
      gtd_provider_todo_txt_load_source (self);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtd_provider_todo_txt_class_init (GtdProviderTodoTxtClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gtd_provider_todo_txt_finalize;
  object_class->get_property = gtd_provider_todo_txt_get_property;
  object_class->set_property = gtd_provider_todo_txt_set_property;

  g_object_class_install_property (object_class,
                                   PROP_SOURCE,
                                   g_param_spec_string ("source",
                                                        "Source file",
                                                        "The Todo.txt source file",
                                                         NULL,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_override_property (object_class, PROP_DESCRIPTION, "description");
  g_object_class_override_property (object_class, PROP_ENABLED, "enabled");
  g_object_class_override_property (object_class, PROP_ICON, "icon");
  g_object_class_override_property (object_class, PROP_ID, "id");
  g_object_class_override_property (object_class, PROP_NAME, "name");
}

static void
gtd_provider_todo_txt_init (GtdProviderTodoTxt *self)
{
  gtd_object_set_ready (GTD_OBJECT (self), TRUE);

  self->no_of_lines = 0;
  self->lists = g_hash_table_new ((GHashFunc) g_str_hash,
                                       (GEqualFunc) g_str_equal);
  self->root_tasks = g_hash_table_new ((GHashFunc) g_str_hash,
                                            (GEqualFunc) g_str_equal);

  /* icon */
  self->icon = G_ICON (g_themed_icon_new_with_default_fallbacks ("computer-symbolic"));
}

void
gtd_provider_todo_txt_set_monitor (GtdProviderTodoTxt *self,
                                   GFileMonitor       *monitor)
{
  g_return_if_fail (GTD_IS_PROVIDER_TODO_TXT (self));

  self->monitor = monitor;
}
