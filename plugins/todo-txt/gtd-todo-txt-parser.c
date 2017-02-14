/* gtd-task-list.c
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

#include <glib/gi18n.h>
#include <gtd-todo-txt-parser.h>

struct _GtdTodoTxtParser
{
  GtdObject          parent;
};

enum {
  TASK_COMPLETE,
  TASK_PRIORITY,
  TASK_DATE,
  TASK_TITLE,
  TASK_LIST_NAME,
  ROOT_TASK_NAME,
  TASK_DUE_DATE
};

G_DEFINE_TYPE (GtdTodoTxtParser, gtd_todo_txt_parser, GTD_TYPE_OBJECT);

struct _TaskData
{
  gchar                   *root_task_name;
  gchar                   *task_list_name;
  gchar                   *title;

  GDateTime               *creation_date;
  GDateTime               *completion_date;
  GDateTime               *due_date;

  gboolean                 is_subtask;
  gboolean                 is_task_completed;
  gint                     priority;

};

static TaskData*
task_data_new (void)
{
  TaskData *tdata;

  tdata = g_new0 (TaskData, 1);

  return tdata;
}

gint
gtd_todo_txt_parser_get_priority (gchar *token)
{
  switch (token[1])
    {
    case 'A':
      return 3;
      break;

    case 'B':
      return 2;
      break;

    case 'C':
      return 1;
      break;

    default:
      return 0;
    }

  return 0;
}

GDateTime*
gtd_todo_txt_parser_get_date (gchar *token)
{
  GDate     *date = NULL;
  GDateTime *dt = NULL;
  gint       year;
  gint       month;
  gint       day;

  date = g_date_new ();

  g_date_set_parse (date, token);

  if (!g_date_valid (date))
    return NULL;

  year = g_date_get_year (date);
  month = g_date_get_month (date);
  day = g_date_get_day (date);

  dt = g_date_time_new_utc (year,
                            month,
                            day,
                            0, 0, 0);
  g_date_free (date);

  return dt;
}

gboolean
gtd_todo_txt_parser_is_date (gchar *dt)
{
  GDate   *date = NULL;

  date = g_date_new ();
  g_date_set_parse (date, dt);

  if (!g_date_valid (date))
    return FALSE;

  g_date_free (date);

  return TRUE;
}

gboolean
gtd_todo_txt_parser_is_word (gchar *token)
{
  guint pos;

  for (pos = 0; pos < strlen (token); pos++)
    {
      if (!g_unichar_isalnum (token[pos]))
        return FALSE;
    }

  return TRUE;
}

gint
gtd_todo_txt_parser_get_token_id (gchar *token,
                                  gint last_read)
{
  gint token_length;

  token_length = strlen (token);

  if (!g_strcmp0 (token, "x"))
    return TASK_COMPLETE;

  if (token_length == 3)
    {
      if (token[0] == '(' && token[2] == ')')
        return TASK_PRIORITY;
    }

  if (!g_str_has_prefix (token , "due:") &&
      gtd_todo_txt_parser_is_date (token))
    return TASK_DATE;


  if (gtd_todo_txt_parser_is_word (token) &&
      (last_read == TASK_DATE ||
       last_read == TASK_PRIORITY ||
       last_read == TASK_COMPLETE||
       last_read == TASK_TITLE))
    return TASK_TITLE;

  if (token_length > 1)
    {
      if (token[0] == '@')
        return TASK_LIST_NAME;
    }

  if (token_length > 1)
    {
      if (token[0] == '+')
        return ROOT_TASK_NAME;
    }

  if (gtd_todo_txt_parser_is_word (token) &&
      last_read == TASK_LIST_NAME)
    return TASK_LIST_NAME;

  if (gtd_todo_txt_parser_is_word (token) &&
      last_read == ROOT_TASK_NAME)
    return ROOT_TASK_NAME;

  if (g_str_has_prefix (token , "due:"))
    return TASK_DUE_DATE;



  return -1;
}

TaskData*
gtd_todo_txt_parser_parse_tokens (GList *tk)
{
  GDateTime *dt;
  TaskData *td;
  GList *it;
  gint last_read_token;
  gint token_id;

  dt = NULL;
  it = NULL;
  td = task_data_new ();

  last_read_token = TASK_COMPLETE;

  for (it = tk; it != NULL; it = it->next)
    {
      gchar *str;

      str = it->data;
      token_id = gtd_todo_txt_parser_get_token_id (it->data, last_read_token);

      switch (token_id)
        {
        case TASK_COMPLETE:
          last_read_token = TASK_COMPLETE;
          td->is_task_completed = TRUE;
          break;

        case TASK_PRIORITY:
          last_read_token = TASK_PRIORITY;
          td->priority = gtd_todo_txt_parser_get_priority (it->data);
          break;

        case TASK_DATE:
          last_read_token = TASK_DATE;
          dt = gtd_todo_txt_parser_get_date (it->data);
          td->creation_date = dt;
          break;

        case TASK_TITLE:
          last_read_token = TASK_TITLE;
          if (td->title == NULL)
            td->title = g_strdup (it->data);
          else
            {
              char *temp = td->title;
              td->title = g_strconcat (td->title, " ",it->data, NULL);
              g_free (temp);
            }
          break;

        case TASK_LIST_NAME:
          last_read_token = TASK_LIST_NAME;
          if (td->task_list_name == NULL)
            td->task_list_name = g_strdup (&str[1]);
          else
            {
              gchar *temp = td->task_list_name;
              td->task_list_name = g_strconcat (td->task_list_name, " ",it->data, NULL);
              g_free (temp);
            }
          break;

        case ROOT_TASK_NAME:
          last_read_token = ROOT_TASK_NAME;
          if (td->root_task_name == NULL)
            td->root_task_name = g_strdup (&str[1]);
          else
            {
              gchar *temp = td->root_task_name;
              td->root_task_name = g_strconcat (td->root_task_name, " ",it->data, NULL);
              g_free (temp);
            }
          td->is_subtask = TRUE;
          break;

        case TASK_DUE_DATE:
          last_read_token = TASK_DUE_DATE;
          dt = gtd_todo_txt_parser_get_date (&str[4]);
          td->due_date = dt;
          break;

        default:
          return NULL;
        }
    }
  return td;
}

gboolean
gtd_todo_txt_parser_validate_token_format (GList *tokens)
{
  GList *it = NULL;
  gint token_id;
  gint position = 0;

  gboolean complete_tk = FALSE;
  gboolean priority_tk = FALSE;
  gboolean task_list_name_tk = FALSE;

  gint last_read = TASK_COMPLETE;

  for (it = tokens; it != NULL; it = it->next)
    {
      gchar *str;

      str = it->data;
      token_id = gtd_todo_txt_parser_get_token_id (it->data, last_read);
      position++;

      switch (token_id)
        {
        case TASK_COMPLETE:
          last_read = TASK_COMPLETE;

          if (position != 1)
            return FALSE;
          else
            complete_tk = TRUE;

          break;

        case TASK_PRIORITY:
          last_read = TASK_PRIORITY;

          if (position != (complete_tk + 1))
            return FALSE;
          else
            priority_tk = TRUE;

          break;

        case TASK_DATE:
          last_read = TASK_DATE;

          if (position != (complete_tk + priority_tk + 1))
            return FALSE;

          if (!gtd_todo_txt_parser_is_date (it->data))
            {
              gtd_manager_emit_error_message (gtd_manager_get_default (),
                                             _("Incorrect date"),
                                             _("Please make sure the date in Todo.txt is valid."));
              return FALSE;
            }

          break;

        case TASK_TITLE:
          last_read = TASK_TITLE;
          break;

        case TASK_LIST_NAME:
          task_list_name_tk = TRUE;
          last_read = TASK_LIST_NAME;
          break;

        case ROOT_TASK_NAME:
          last_read = ROOT_TASK_NAME;
          break;

        case TASK_DUE_DATE:
          last_read = TASK_DUE_DATE;

          if (!gtd_todo_txt_parser_is_date (&str[4]))
            return FALSE;

          break;

        default:
          gtd_manager_emit_error_message (gtd_manager_get_default (),
                                          _("Unrecognized token in a Todo.txt line"),
                                          _("To Do cannot recognize some tags in your Todo.txt file. Some tasks may not be loaded"));
          return FALSE;
          break;
        }
    }

  if (!task_list_name_tk)
    {
      gtd_manager_emit_error_message (gtd_manager_get_default (),
                                      _("No task list found for some tasks"),
                                      _("Some of the tasks in your Todo.txt file do not have a task list. To Do supports tasks with a task list. Please add a list to all your tasks"));
      return FALSE;
    }

  return TRUE;
}

GList*
gtd_todo_txt_parser_tokenize (const gchar *line)
{
  GList *tokens = NULL;
  gchar **token = NULL;

  token = g_strsplit (line, " ", -1);

  while (*token)
  {
    g_strstrip (*token);
    tokens = g_list_prepend (tokens, g_strdup(*token));
    token++;
  }

  tokens = g_list_reverse (tokens);

  return tokens;
}

GList*
gtd_todo_txt_parser_get_list_updated_token (GtdTaskList *list,
                                            gchar       *line)
{
  gint last_read_token = TASK_COMPLETE;
  gboolean list_name_updated = FALSE;
  GList *tokens = NULL;
  GList *it = NULL;

  tokens = gtd_todo_txt_parser_tokenize (line);

  for (it = tokens; it != NULL; it = it->next)
    {
      gint token_id;

      token_id = gtd_todo_txt_parser_get_token_id (it->data, last_read_token);
      last_read_token = token_id;
      if (token_id == TASK_LIST_NAME)
        {
          if (list_name_updated)
            {
              it = g_list_remove (tokens, it->data);
              last_read_token = TASK_COMPLETE;
            }

          else
            {
              it->data = g_strconcat ("@", gtd_task_list_get_name(list), NULL);
              list_name_updated = TRUE;
            }
        }
    }

  return tokens;
}

GList*
gtd_todo_txt_parser_get_task_line (GtdTask *task)
{
  GtdTaskList *list;
  GDateTime   *dt;
  GtdTask *parent;
  GList *tokens = NULL;
  gint priority;
  gboolean is_complete;
  const gchar *list_name;
  const gchar *title;

  is_complete = gtd_task_get_complete (task);
  title = gtd_task_get_title (task);
  priority = gtd_task_get_priority (task);
  dt = gtd_task_get_due_date (task);
  list = gtd_task_get_list (task);
  parent = gtd_task_get_parent (task);

  list_name = gtd_task_list_get_name (list);

  if (is_complete)
    tokens = g_list_append (tokens, g_strdup ("x"));

  if (priority)
    {
      if (priority == 1)
        tokens = g_list_append (tokens, g_strdup ("(C)"));
      else if (priority == 2)
        tokens = g_list_append (tokens, g_strdup ("(B)"));
      else if (priority == 3)
        tokens = g_list_append (tokens, g_strdup ("(A)"));
    }

  tokens = g_list_append (tokens, g_strdup (title));
  tokens = g_list_append (tokens, g_strconcat ("@", list_name, NULL));

  if (parent)
    tokens = g_list_append (tokens, g_strconcat ("+", gtd_task_get_title (parent), NULL));

  if (dt)
    tokens = g_list_append (tokens, g_strconcat ("due:",g_date_time_format (dt, "%F"),NULL));

  return tokens;
}

/* Accessor Methods for the TaskData Structure */

gchar*
gtd_todo_txt_parser_task_data_get_root_task_name (TaskData *td)
{
  return td->root_task_name;
}

gchar*
gtd_todo_txt_parser_task_data_get_task_list_name (TaskData *td)
{
  return td->task_list_name;
}

gchar*
gtd_todo_txt_parser_task_data_get_title (TaskData *td)
{
  return td->title;
}

GDateTime*
gtd_todo_txt_parser_task_data_get_due_date (TaskData *td)
{
  return td->due_date;
}

gboolean
gtd_todo_txt_parser_task_data_is_subtask (TaskData *td)
{
  return td->is_subtask;
}

gboolean
gtd_todo_txt_parser_task_data_is_task_completed (TaskData *td)
{
  return td->is_task_completed;
}

gint
gtd_todo_txt_parser_task_data_get_priority (TaskData *td)
{
  return td->priority;
}

static void
gtd_todo_txt_parser_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
gtd_todo_txt_parser_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
gtd_todo_txt_parser_class_init (GtdTodoTxtParserClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = gtd_todo_txt_parser_get_property;
  object_class->set_property = gtd_todo_txt_parser_set_property;

}

static void
gtd_todo_txt_parser_init (GtdTodoTxtParser *self)
{
  ;
}
