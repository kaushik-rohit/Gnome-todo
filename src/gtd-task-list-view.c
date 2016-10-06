/* gtd-task-list-view.c
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

#include "gtd-arrow-frame.h"
#include "gtd-edit-pane.h"
#include "gtd-task-list-view.h"
#include "gtd-manager.h"
#include "gtd-notification.h"
#include "gtd-provider.h"
#include "gtd-task.h"
#include "gtd-task-list.h"
#include "gtd-task-row.h"
#include "gtd-window.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

typedef struct
{
  GtdArrowFrame         *arrow_frame;
  GtdEditPane           *edit_pane;
  GtkRevealer           *edit_revealer;
  GtkWidget             *empty_box;
  GtkListBox            *listbox;
  GtdTaskRow            *new_task_row;
  GtkRevealer           *revealer;
  GtkImage              *done_image;
  GtkLabel              *done_label;
  GtkScrolledWindow     *viewport;

  /* internal */
  gboolean               can_toggle;
  gint                   complete_tasks;
  gboolean               show_list_name;
  gboolean               show_completed;
  GList                 *list;
  GtdTaskList           *task_list;
  GDateTime             *default_date;

  /* color provider */
  GtkCssProvider        *color_provider;
  GdkRGBA               *color;

  /* action */
  GActionGroup          *action_group;

  /* Custom header function data */
  GtdTaskListViewHeaderFunc header_func;
  gpointer                  header_user_data;

  /* Custom sorting function data */
  GtdTaskListViewSortFunc sort_func;
  gpointer                sort_user_data;
} GtdTaskListViewPrivate;

struct _GtdTaskListView
{
  GtkOverlay          parent;

  /*<private>*/
  GtdTaskListViewPrivate *priv;
};

#define COLOR_TEMPLATE "viewport {background-color: %s;}"
#define LUMINANCE(c)   (0.299 * c->red + 0.587 * c->green + 0.114 * c->blue)

#define TASK_REMOVED_NOTIFICATION_ID             "task-removed-id"

/* prototypes */
static void             gtd_task_list_view__clear_completed_tasks    (GSimpleAction     *simple,
                                                                      GVariant          *parameter,
                                                                      gpointer           user_data);

static void             gtd_task_list_view__remove_row_for_task      (GtdTaskListView   *view,
                                                                      GtdTask           *task);

static void             gtd_task_list_view__remove_task              (GtdTaskListView   *view,
                                                                      GtdTask           *task);

static void             gtd_task_list_view__task_completed            (GObject          *object,
                                                                       GParamSpec       *spec,
                                                                       gpointer          user_data);

static void             gtd_task_list_view__update_done_label         (GtdTaskListView   *view);

G_DEFINE_TYPE_WITH_PRIVATE (GtdTaskListView, gtd_task_list_view, GTK_TYPE_OVERLAY)

static const GActionEntry gtd_task_list_view_entries[] = {
  { "clear-completed-tasks", gtd_task_list_view__clear_completed_tasks },
};

typedef struct
{
  GtdTaskListView *view;
  GtdTask         *task;
} RemoveTaskData;

enum {
  PROP_0,
  PROP_COLOR,
  PROP_SHOW_COMPLETED,
  PROP_SHOW_LIST_NAME,
  PROP_SHOW_NEW_TASK_ROW,
  LAST_PROP
};

static void
real_save_task (GtdTaskListView *self,
                GtdTask         *task)
{
  GtdTaskListViewPrivate *priv;
  GtdTaskList *list;

  priv = self->priv;
  list = gtd_task_get_list (task);

  /*
   * This will emit GtdTaskList::task-added and we'll readd
   * to the list.
   */
  gtd_task_list_save_task (list, task);

  if (priv->task_list != list && priv->task_list)
    gtd_task_list_save_task (priv->task_list, task);
}


static void
remove_task_action (GtdNotification *notification,
                    gpointer         user_data)
{
  RemoveTaskData *data = user_data;

  gtd_manager_remove_task (gtd_manager_get_default (), data->task);

  g_free (data);
}

static void
undo_remove_task_action (GtdNotification *notification,
                         gpointer         user_data)
{
  RemoveTaskData *data = user_data;

  real_save_task (data->view, data->task);

  g_free (data);
}

static void
internal_header_func (GtdTaskRow      *row,
                      GtdTaskRow      *before,
                      GtdTaskListView *view)
{
  GtdTask *row_task;
  GtdTask *before_task;

  if (!view->priv->header_func || row == view->priv->new_task_row)
    return;

  row_task = before_task = NULL;

  if (row)
    row_task = gtd_task_row_get_task (row);

  if (before)
    before_task = gtd_task_row_get_task (before);

  view->priv->header_func (GTK_LIST_BOX_ROW (row),
                           row_task,
                           GTK_LIST_BOX_ROW (before),
                           before_task,
                           view->priv->header_user_data);
}

static gint
internal_sort_func (GtdTaskRow      *row1,
                    GtdTaskRow      *row2,
                    GtdTaskListView *view)
{
  GtdTask *row1_task;
  GtdTask *row2_task;

  if (!view->priv->sort_func)
    return 0;

  if (gtd_task_row_get_new_task_mode (row1))
    return 1;
  else if (gtd_task_row_get_new_task_mode (row2))
    return -1;

  row1_task = row2_task = NULL;

  if (row1)
    row1_task = gtd_task_row_get_task (row1);

  if (row2)
    row2_task = gtd_task_row_get_task (row2);

  return view->priv->sort_func (GTK_LIST_BOX_ROW (row1),
                                row1_task,
                                GTK_LIST_BOX_ROW (row2),
                                row2_task,
                                view->priv->header_user_data);
}

static void
update_font_color (GtdTaskListView *view)
{
  GtdTaskListViewPrivate *priv;

  g_return_if_fail (GTD_IS_TASK_LIST_VIEW (view));

  priv = view->priv;

  if (priv->task_list)
    {
      GtkStyleContext *context;
      GdkRGBA *color;

      context = gtk_widget_get_style_context (GTK_WIDGET (view));
      color = gtd_task_list_get_color (priv->task_list);

      if (LUMINANCE (color) < 0.5)
        gtk_style_context_add_class (context, "dark");
      else
        gtk_style_context_remove_class (context, "dark");

      gdk_rgba_free (color);
    }
}

static void
gtd_task_list_view__clear_completed_tasks (GSimpleAction *simple,
                                           GVariant      *parameter,
                                           gpointer       user_data)
{
  GtdTaskListView *view;
  GList *tasks;
  GList *l;

  view = GTD_TASK_LIST_VIEW (user_data);
  tasks = gtd_task_list_view_get_list (view);

  for (l = tasks; l != NULL; l = l->next)
    {
      if (gtd_task_get_complete (l->data))
        {
          GtdTaskList *list;

          list = gtd_task_get_list (l->data);

          gtd_task_list_remove_task (list, l->data);
          gtd_manager_remove_task (gtd_manager_get_default (), l->data);
        }
    }

  gtd_task_list_view__update_done_label (view);

  g_list_free (tasks);
}

static void
gtd_task_list_view__update_empty_state (GtdTaskListView *view)
{
  GtdTaskListViewPrivate *priv;
  gboolean is_empty;
  GList *tasks;
  GList *l;

  g_return_if_fail (GTD_IS_TASK_LIST_VIEW (view));

  priv = view->priv;
  is_empty = TRUE;
  tasks = gtd_task_list_view_get_list (view);

  for (l = tasks; l != NULL; l = l->next)
    {
      if (!gtd_task_get_complete (l->data) ||
          (priv->show_completed && gtd_task_get_complete (l->data)))
        {
          is_empty = FALSE;
          break;
        }
    }

  gtk_widget_set_visible (view->priv->empty_box, is_empty);

  g_list_free (tasks);
}

static void
gtd_task_list_view__remove_task_cb (GtdEditPane *pane,
                                    GtdTask     *task,
                                    gpointer     user_data)
{
  GtdTaskListViewPrivate *priv;
  GtdNotification *notification;
  RemoveTaskData *data;
  GtdTaskList *list;
  GtdWindow *window;
  gchar *text;

  g_return_if_fail (GTD_IS_TASK_LIST_VIEW (user_data));

  priv = GTD_TASK_LIST_VIEW (user_data)->priv;
  text = g_strdup_printf (_("Task <b>%s</b> removed"), gtd_task_get_title (task));
  window = GTD_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (user_data)));
  list = gtd_task_get_list (task);

  data = g_new0 (RemoveTaskData, 1);
  data->view = user_data;
  data->task = task;

  /* Remove the task from the list */
  gtd_task_list_remove_task (list, task);

  /*
   * When we're dealing with the special lists (Today & Scheduled),
   * the task's list is different from the current list. We want to
   * remove the task from ~both~ lists.
   */
  if (priv->task_list != list && priv->task_list)
    gtd_task_list_remove_task (priv->task_list, task);

  gtd_task_list_view__remove_row_for_task (GTD_TASK_LIST_VIEW (user_data), task);

  gtk_revealer_set_reveal_child (priv->edit_revealer, FALSE);

  /* Notify about the removal */
  notification = gtd_notification_new (text, 7500.0);

  gtd_notification_set_primary_action (notification,
                                       (GtdNotificationActionFunc) remove_task_action,
                                       data);

  gtd_notification_set_secondary_action (notification,
                                         _("Undo"),
                                         (GtdNotificationActionFunc) undo_remove_task_action,
                                         data);

  gtd_window_notify (window, notification);

  g_free (text);
}

static void
gtd_task_list_view__edit_task_finished (GtdEditPane *pane,
                                        GtdTask     *task,
                                        gpointer     user_data)
{
  GtdTaskListViewPrivate *priv;

  g_return_if_fail (GTD_IS_TASK (task));
  g_return_if_fail (GTD_IS_EDIT_PANE (pane));
  g_return_if_fail (GTD_IS_TASK_LIST_VIEW (user_data));

  priv = GTD_TASK_LIST_VIEW (user_data)->priv;

  gtk_revealer_set_reveal_child (priv->edit_revealer, FALSE);

  gtd_task_save (task);

  gtd_manager_update_task (gtd_manager_get_default (), task);
  real_save_task (GTD_TASK_LIST_VIEW (user_data), task);

  gtk_list_box_invalidate_sort (priv->listbox);
}

static void
gtd_task_list_view__color_changed (GtdTaskListView *self)
{
  GtdTaskListViewPrivate *priv = GTD_TASK_LIST_VIEW (self)->priv;
  gchar *color_str;
  gchar *parsed_css;

  /* Add the color to provider */
  if (priv->color)
    {
      color_str = gdk_rgba_to_string (priv->color);
    }
  else
    {
      GdkRGBA *color;

      color = gtd_task_list_get_color (GTD_TASK_LIST (priv->task_list));
      color_str = gdk_rgba_to_string (color);

      gdk_rgba_free (color);
    }

  parsed_css = g_strdup_printf (COLOR_TEMPLATE, color_str);

  gtk_css_provider_load_from_data (priv->color_provider,
                                   parsed_css,
                                   -1,
                                   NULL);

  update_font_color (self);

  g_free (color_str);
}

static void
gtd_task_list_view__update_done_label (GtdTaskListView *view)
{
  gchar *new_label;

  g_return_if_fail (GTD_IS_TASK_LIST_VIEW (view));

  gtk_revealer_set_reveal_child (GTK_REVEALER (view->priv->revealer), view->priv->complete_tasks > 0);

  new_label = g_strdup_printf ("%s (%d)",
                               _("Done"),
                               view->priv->complete_tasks);

  gtk_label_set_label (view->priv->done_label, new_label);

  g_free (new_label);
}

static gboolean
can_toggle_show_completed (GtdTaskListView *view)
{
  view->priv->can_toggle = TRUE;
  return G_SOURCE_REMOVE;
}

static void
gtd_task_list_view__done_button_clicked (GtkButton *button,
                                         gpointer   user_data)
{
  GtdTaskListView *view = GTD_TASK_LIST_VIEW (user_data);
  gboolean show_completed;

  g_return_if_fail (GTD_IS_TASK_LIST_VIEW (view));

  if (!view->priv->can_toggle)
    return;

  /*
   * The can_toggle bitfield is needed because the user
   * can click mindlessly the Done button, while the row
   * animations are not finished. While the animation is
   * running, we ignore other clicks.
   */
  view->priv->can_toggle = FALSE;

  show_completed = view->priv->show_completed;

  gtd_task_list_view_set_show_completed (view, !show_completed);

  g_timeout_add (205,
                 (GSourceFunc) can_toggle_show_completed,
                 user_data);
}

static gint
gtd_task_list_view__listbox_sort_func (GtdTaskRow *row1,
                                       GtdTaskRow *row2,
                                       gpointer    user_data)
{
  g_return_val_if_fail (GTD_IS_TASK_ROW (row1), 0);
  g_return_val_if_fail (GTD_IS_TASK_ROW (row2), 0);

  if (gtd_task_row_get_new_task_mode (row1))
    return 1;
  else if (gtd_task_row_get_new_task_mode (row2))
    return -1;
  else
    return gtd_task_compare (gtd_task_row_get_task (row1), gtd_task_row_get_task (row2));
}

static void
remove_task (GtdTaskListView *view,
             GtdTask         *task)
{
  GtdTaskListViewPrivate *priv = view->priv;
  GList *children;
  GList *l;

  gtd_arrow_frame_set_row (view->priv->arrow_frame, NULL);

  children = gtk_container_get_children (GTK_CONTAINER (view->priv->listbox));

  for (l = children; l != NULL; l = l->next)
    {
      if (l->data != priv->new_task_row &&
          gtd_task_row_get_task (l->data) == task)
        {
          if (gtd_task_get_complete (task))
            priv->complete_tasks--;

          g_signal_handlers_disconnect_by_func (gtd_task_row_get_task (l->data),
                                                gtd_task_list_view__task_completed,
                                                view);
          gtk_widget_destroy (l->data);
        }
    }

  gtk_revealer_set_reveal_child (priv->revealer, FALSE);
  gtk_revealer_set_reveal_child (priv->edit_revealer, FALSE);

  g_list_free (children);
}

static void
gtd_task_list_view__row_activated (GtkListBox *listbox,
                                   GtdTaskRow *row,
                                   gpointer    user_data)
{
  GtdTaskListViewPrivate *priv = GTD_TASK_LIST_VIEW (user_data)->priv;

  if (row == priv->new_task_row)
    return;

  gtd_edit_pane_set_task (priv->edit_pane, gtd_task_row_get_task (row));

  gtk_revealer_set_reveal_child (priv->edit_revealer, TRUE);
  gtd_arrow_frame_set_row (priv->arrow_frame, row);
}

static void
gtd_task_list_view__add_task (GtdTaskListView *view,
                              GtdTask         *task)
{
  GtdTaskListViewPrivate *priv = view->priv;
  GtkWidget *new_row;

  g_return_if_fail (GTD_IS_TASK_LIST_VIEW (view));
  g_return_if_fail (GTD_IS_TASK (task));

  new_row = gtd_task_row_new (task);

  gtd_task_row_set_list_name_visible (GTD_TASK_ROW (new_row), priv->show_list_name);

  if (!gtd_task_get_complete (task))
    {
      gtk_list_box_insert (priv->listbox,
                           new_row,
                           0);
      gtd_task_row_reveal (GTD_TASK_ROW (new_row));
    }
  else
    {
      priv->complete_tasks++;

      gtd_task_list_view__update_done_label (view);

      if (!gtk_revealer_get_reveal_child (priv->revealer))
        gtk_revealer_set_reveal_child (priv->revealer, TRUE);
    }

  /* Check if it should show the empty state */
  gtd_task_list_view__update_empty_state (view);
}

static void
gtd_task_list_view__remove_row_for_task (GtdTaskListView *view,
                                         GtdTask         *task)
{
  GtdTaskListViewPrivate *priv = view->priv;
  GList *children;
  GList *l;

  g_return_if_fail (GTD_IS_TASK_LIST_VIEW (view));
  g_return_if_fail (GTD_IS_TASK (task));

  children = gtk_container_get_children (GTK_CONTAINER (priv->listbox));

  for (l = children; l != NULL; l = l->next)
    {
      if (!gtd_task_row_get_new_task_mode (l->data) &&
          gtd_task_row_get_task (l->data) == task)
        {
          gtk_widget_destroy (l->data);
        }
    }

  g_list_free (children);
}

static void
gtd_task_list_view__remove_task (GtdTaskListView *view,
                                 GtdTask         *task)
{
  /* Remove the correspondent row */
  gtd_task_list_view__remove_row_for_task (view, task);

  /* Update the "Done" label */
  if (gtd_task_get_complete (task))
    {
      view->priv->complete_tasks--;
      gtd_task_list_view__update_done_label (view);
    }

  /* Check if it should show the empty state */
  gtd_task_list_view__update_empty_state (view);
}

static void
gtd_task_list_view__task_completed (GObject    *object,
                                    GParamSpec *spec,
                                    gpointer    user_data)
{
  GtdTaskListViewPrivate *priv = GTD_TASK_LIST_VIEW (user_data)->priv;
  GtdTask *task = GTD_TASK (object);
  gboolean task_complete;

  g_return_if_fail (GTD_IS_TASK (object));
  g_return_if_fail (GTD_IS_TASK_LIST_VIEW (user_data));

  task_complete = gtd_task_get_complete (task);

  gtd_manager_update_task (gtd_manager_get_default (), task);
  real_save_task (GTD_TASK_LIST_VIEW (user_data), task);

  if (task_complete)
    priv->complete_tasks++;
  else
    priv->complete_tasks--;

  /*
   * If we're editing the task and it get completed, hide the edit
   * pane and the task.
   */
  if (task_complete &&
      task == gtd_edit_pane_get_task (priv->edit_pane))
    {
      gtk_revealer_set_reveal_child (priv->edit_revealer, FALSE);
      gtd_edit_pane_set_task (priv->edit_pane, NULL);
    }

  gtd_task_list_view__update_done_label (GTD_TASK_LIST_VIEW (user_data));

  if (!priv->show_completed)
    {
      if (task_complete)
        gtd_task_list_view__remove_row_for_task (GTD_TASK_LIST_VIEW (user_data), task);
      else
        gtd_task_list_view__add_task (GTD_TASK_LIST_VIEW (user_data), task);
    }
  else
    {
      gtk_list_box_invalidate_sort (priv->listbox);
    }
}

static void
gtd_task_list_view__task_added (GtdTaskList *list,
                                GtdTask     *task,
                                gpointer     user_data)
{
  g_return_if_fail (GTD_IS_TASK_LIST_VIEW (user_data));
  g_return_if_fail (GTD_IS_TASK_LIST (list));
  g_return_if_fail (GTD_IS_TASK (task));

  /* Add the new task to the list */
  gtd_task_list_view__add_task (GTD_TASK_LIST_VIEW (user_data), task);

  g_signal_connect (task,
                    "notify::complete",
                    G_CALLBACK (gtd_task_list_view__task_completed),
                    user_data);
}

static void
gtd_task_list_view__create_task (GtdTaskRow *row,
                                 GtdTask    *task,
                                 gpointer    user_data)
{
  GtdTaskListViewPrivate *priv;
  GtdTaskList *list;

  priv = GTD_TASK_LIST_VIEW (user_data)->priv;
  list = priv->task_list;

  /*
   * If there is no current list set, use the default list from the
   * default provider.
   */
  if (!list)
    {
      GtdProvider *provider;

      provider = gtd_manager_get_default_provider (gtd_manager_get_default ());
      list = gtd_provider_get_default_task_list (provider);
    }

  g_return_if_fail (GTD_IS_TASK_LIST (list));

  /*
   * Newly created tasks are not aware of
   * their parent lists.
   */
  gtd_task_set_list (task, list);

  if (priv->default_date)
    gtd_task_set_due_date (task, priv->default_date);

  gtd_task_list_save_task (list, task);
  gtd_manager_create_task (gtd_manager_get_default (), task);
}

static void
gtd_task_list_view_finalize (GObject *object)
{
  GtdTaskListViewPrivate *priv = GTD_TASK_LIST_VIEW (object)->priv;

  g_clear_pointer (&priv->default_date, g_date_time_unref);
  g_clear_pointer (&priv->list, g_list_free);

  G_OBJECT_CLASS (gtd_task_list_view_parent_class)->finalize (object);
}

static void
gtd_task_list_view_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GtdTaskListView *self = GTD_TASK_LIST_VIEW (object);

  switch (prop_id)
    {
    case PROP_SHOW_COMPLETED:
      g_value_set_boolean (value, self->priv->show_completed);
      break;

    case PROP_SHOW_LIST_NAME:
      g_value_set_boolean (value, self->priv->show_list_name);
      break;

    case PROP_SHOW_NEW_TASK_ROW:
      g_value_set_boolean (value, gtk_widget_get_visible (GTK_WIDGET (self->priv->new_task_row)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtd_task_list_view_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GtdTaskListView *self = GTD_TASK_LIST_VIEW (object);

  switch (prop_id)
    {
    case PROP_SHOW_COMPLETED:
      gtd_task_list_view_set_show_completed (self, g_value_get_boolean (value));
      break;

    case PROP_SHOW_LIST_NAME:
      gtd_task_list_view_set_show_list_name (self, g_value_get_boolean (value));
      break;

    case PROP_SHOW_NEW_TASK_ROW:
      gtd_task_list_view_set_show_new_task_row (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtd_task_list_view_constructed (GObject *object)
{
  GtdTaskListView *self = GTD_TASK_LIST_VIEW (object);

  G_OBJECT_CLASS (gtd_task_list_view_parent_class)->constructed (object);

  /* action_group */
  self->priv->action_group = G_ACTION_GROUP (g_simple_action_group_new ());

  g_action_map_add_action_entries (G_ACTION_MAP (self->priv->action_group),
                                   gtd_task_list_view_entries,
                                   G_N_ELEMENTS (gtd_task_list_view_entries),
                                   object);

  /* css provider */
  self->priv->color_provider = gtk_css_provider_new ();

  gtk_style_context_add_provider (gtk_widget_get_style_context (GTK_WIDGET (self->priv->viewport)),
                                  GTK_STYLE_PROVIDER (self->priv->color_provider),
                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 2);

  /* show a nifty separator between lines */
  gtk_list_box_set_sort_func (self->priv->listbox,
                              (GtkListBoxSortFunc) gtd_task_list_view__listbox_sort_func,
                              NULL,
                              NULL);
}

static void
gtd_task_list_view_map (GtkWidget *widget)
{
  GtdTaskListViewPrivate *priv;
  GtkWidget *window;

  GTK_WIDGET_CLASS (gtd_task_list_view_parent_class)->map (widget);

  priv = GTD_TASK_LIST_VIEW (widget)->priv;
  window = gtk_widget_get_toplevel (widget);

  /* Clear previously added "list" actions */
  gtk_widget_insert_action_group (window,
                                  "list",
                                  NULL);

  /* Add this instance's action group */
  gtk_widget_insert_action_group (window,
                                  "list",
                                  priv->action_group);
}

static void
gtd_task_list_view_class_init (GtdTaskListViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = gtd_task_list_view_finalize;
  object_class->constructed = gtd_task_list_view_constructed;
  object_class->get_property = gtd_task_list_view_get_property;
  object_class->set_property = gtd_task_list_view_set_property;

  widget_class->map = gtd_task_list_view_map;

  g_type_ensure (GTD_TYPE_TASK_ROW);

  /**
   * GtdTaskListView::color:
   *
   * The custom color of this list. If there is a custom color set,
   * the tasklist's color is ignored.
   */
  g_object_class_install_property (
        object_class,
        PROP_COLOR,
        g_param_spec_boxed ("color",
                            "Color of the task list view",
                            "The custom color of this task list view",
                            GDK_TYPE_RGBA,
                            G_PARAM_READWRITE));

  /**
   * GtdTaskListView::show-new-task-row:
   *
   * Whether the list shows the "New Task" row or not.
   */
  g_object_class_install_property (
        object_class,
        PROP_SHOW_NEW_TASK_ROW,
        g_param_spec_boolean ("show-new-task-row",
                              "Whether it shows the New Task row",
                              "Whether the list shows the New Task row, or not",
                              TRUE,
                              G_PARAM_READWRITE));

  /**
   * GtdTaskListView::show-list-name:
   *
   * Whether the task rows should show the list name.
   */
  g_object_class_install_property (
        object_class,
        PROP_SHOW_LIST_NAME,
        g_param_spec_boolean ("show-list-name",
                              "Whether task rows show the list name",
                              "Whether task rows show the list name at the end of the row",
                              FALSE,
                              G_PARAM_READWRITE));

  /**
   * GtdTaskListView::show-completed:
   *
   * Whether completed tasks are shown.
   */
  g_object_class_install_property (
        object_class,
        PROP_SHOW_COMPLETED,
        g_param_spec_boolean ("show-completed",
                              "Whether completed tasks are shown",
                              "Whether completed tasks are visible or not",
                              FALSE,
                              G_PARAM_READWRITE));

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/todo/ui/list-view.ui");

  gtk_widget_class_bind_template_child_private (widget_class, GtdTaskListView, arrow_frame);
  gtk_widget_class_bind_template_child_private (widget_class, GtdTaskListView, edit_pane);
  gtk_widget_class_bind_template_child_private (widget_class, GtdTaskListView, edit_revealer);
  gtk_widget_class_bind_template_child_private (widget_class, GtdTaskListView, empty_box);
  gtk_widget_class_bind_template_child_private (widget_class, GtdTaskListView, listbox);
  gtk_widget_class_bind_template_child_private (widget_class, GtdTaskListView, revealer);
  gtk_widget_class_bind_template_child_private (widget_class, GtdTaskListView, done_image);
  gtk_widget_class_bind_template_child_private (widget_class, GtdTaskListView, done_label);
  gtk_widget_class_bind_template_child_private (widget_class, GtdTaskListView, new_task_row);
  gtk_widget_class_bind_template_child_private (widget_class, GtdTaskListView, viewport);

  gtk_widget_class_bind_template_callback (widget_class, gtd_task_list_view__create_task);
  gtk_widget_class_bind_template_callback (widget_class, gtd_task_list_view__done_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class, gtd_task_list_view__edit_task_finished);
  gtk_widget_class_bind_template_callback (widget_class, gtd_task_list_view__remove_task_cb);
  gtk_widget_class_bind_template_callback (widget_class, gtd_task_list_view__row_activated);

  gtk_widget_class_set_css_name (widget_class, "task-list-view");
}

static void
gtd_task_list_view_init (GtdTaskListView *self)
{
  self->priv = gtd_task_list_view_get_instance_private (self);
  self->priv->can_toggle = TRUE;

  gtk_widget_init_template (GTK_WIDGET (self));
}

/**
 * gtd_task_list_view_new:
 *
 * Creates a new #GtdTaskListView
 *
 * Returns: (transfer full): a newly allocated #GtdTaskListView
 */
GtkWidget*
gtd_task_list_view_new (void)
{
  return g_object_new (GTD_TYPE_TASK_LIST_VIEW, NULL);
}

/**
 * gtd_task_list_view_get_list:
 * @view: a #GtdTaskListView
 *
 * Retrieves the list of tasks from @view. Note that,
 * if a #GtdTaskList is set, the #GtdTaskList's list
 * of task will be returned.
 *
 * Returns: (element-type Gtd.TaskList) (transfer full): the internal list of
 * tasks. Free with @g_list_free after use.
 */
GList*
gtd_task_list_view_get_list (GtdTaskListView *view)
{
  g_return_val_if_fail (GTD_IS_TASK_LIST_VIEW (view), NULL);

  if (view->priv->task_list)
    return gtd_task_list_get_tasks (view->priv->task_list);
  else if (view->priv->list)
    return g_list_copy (view->priv->list);
  else
    return NULL;
}

/**
 * gtd_task_list_view_set_list:
 * @view: a #GtdTaskListView
 * @list: (element-type Gtd.Task) (nullable): a list of tasks
 *
 * Copies the tasks from @list to @view.
 */
void
gtd_task_list_view_set_list (GtdTaskListView *view,
                             GList           *list)
{
  GtdTaskListViewPrivate *priv;
  GList *l, *old_list;

  g_return_if_fail (GTD_IS_TASK_LIST_VIEW (view));

  priv = view->priv;
  old_list = priv->list;

  /* Remove the tasks that are in the current list, but not in the new list */
  for (l = old_list; l != NULL; l = l->next)
    {
      if (!g_list_find (list, l->data))
        remove_task (view, l->data);
    }

  /* Add the tasks that are in the new list, but not in the current list */
  for (l = list; l != NULL; l = l->next)
    {
      if (g_list_find (old_list, l->data))
        continue;

      gtd_task_list_view__add_task (view, l->data);

      g_signal_connect (l->data,
                        "notify::complete",
                        G_CALLBACK (gtd_task_list_view__task_completed),
                        view);
    }

  g_list_free (old_list);
  priv->list = g_list_copy (list);

  /* Check if it should show the empty state */
  gtd_task_list_view__update_empty_state (view);
}

/**
 * gtd_task_list_view_get_show_new_task_row:
 * @view: a #GtdTaskListView
 *
 * Gets whether @view shows the new task row or not.
 *
 * Returns: %TRUE if @view is shows the new task row, %FALSE otherwise
 */
gboolean
gtd_task_list_view_get_show_new_task_row (GtdTaskListView *self)
{
  g_return_val_if_fail (GTD_IS_TASK_LIST_VIEW (self), FALSE);

  return gtk_widget_get_visible (GTK_WIDGET (self->priv->new_task_row));
}

/**
 * gtd_task_list_view_set_show_new_task_row:
 * @view: a #GtdTaskListView
 *
 * Sets the GtdTaskListView::show-new-task-mode property of @view.
 */
void
gtd_task_list_view_set_show_new_task_row (GtdTaskListView *view,
                                          gboolean         show_new_task_row)
{
  g_return_if_fail (GTD_IS_TASK_LIST_VIEW (view));

  gtk_widget_set_visible (GTK_WIDGET (view->priv->new_task_row), show_new_task_row);
  g_object_notify (G_OBJECT (view), "show-new-task-row");
}

/**
 * gtd_task_list_view_get_task_list:
 * @view: a #GtdTaskListView
 *
 * Retrieves the #GtdTaskList from @view, or %NULL if none was set.
 *
 * Returns: (transfer none): the @GtdTaskList of @view, or %NULL is
 * none was set.
 */
GtdTaskList*
gtd_task_list_view_get_task_list (GtdTaskListView *view)
{
  g_return_val_if_fail (GTD_IS_TASK_LIST_VIEW (view), NULL);

  return view->priv->task_list;
}

/**
 * gtd_task_list_view_set_task_list:
 * @view: a #GtdTaskListView
 * @list: a #GtdTaskList
 *
 * Sets the internal #GtdTaskList of @view.
 */
void
gtd_task_list_view_set_task_list (GtdTaskListView *view,
                                  GtdTaskList     *list)
{
  GtdTaskListViewPrivate *priv = view->priv;

  g_return_if_fail (GTD_IS_TASK_LIST_VIEW (view));
  g_return_if_fail (GTD_IS_TASK_LIST (list));

  if (priv->task_list != list)
    {
      GdkRGBA *color;
      gchar *color_str;
      gchar *parsed_css;
      GList *task_list;

      /*
       * Disconnect the old GtdTaskList signals.
       */
      if (priv->task_list)
        {
          g_signal_handlers_disconnect_by_func (priv->task_list,
                                                gtd_task_list_view__task_added,
                                                view);
          g_signal_handlers_disconnect_by_func (priv->task_list,
                                                gtd_task_list_view__color_changed,
                                                view);
        }

      /* Add the color to provider */
      color = gtd_task_list_get_color (list);
      color_str = gdk_rgba_to_string (color);

      parsed_css = g_strdup_printf (COLOR_TEMPLATE, color_str);

      g_debug ("setting style for provider: %s", parsed_css);

      gtk_css_provider_load_from_data (priv->color_provider,
                                       parsed_css,
                                       -1,
                                       NULL);

      g_free (parsed_css);
      gdk_rgba_free (color);
      g_free (color_str);

      /* Load task */
      priv->task_list = list;

      update_font_color (view);

      /* Add the tasks from the list */
      task_list = gtd_task_list_get_tasks (list);

      gtd_task_list_view_set_list (view, task_list);

      g_list_free (task_list);

      g_signal_connect (list,
                        "task-added",
                        G_CALLBACK (gtd_task_list_view__task_added),
                        view);
      g_signal_connect_swapped (list,
                                "task-removed",
                                G_CALLBACK (gtd_task_list_view__remove_task),
                                view);
      g_signal_connect_swapped (list,
                                "notify::color",
                                G_CALLBACK (gtd_task_list_view__color_changed),
                                view);
    }
}

/**
 * gtd_task_list_view_get_show_list_name:
 * @view: a #GtdTaskListView
 *
 * Whether @view shows the tasks' list names.
 *
 * Returns: %TRUE if @view show the tasks' list names, %FALSE otherwise
 */
gboolean
gtd_task_list_view_get_show_list_name (GtdTaskListView *view)
{
  g_return_val_if_fail (GTD_IS_TASK_LIST_VIEW (view), FALSE);

  return view->priv->show_list_name;
}

/**
 * gtd_task_list_view_set_show_list_name:
 * @view: a #GtdTaskListView
 * @show_list_name: %TRUE to show list names, %FALSE to hide it
 *
 * Whether @view should should it's tasks' list name.
 */
void
gtd_task_list_view_set_show_list_name (GtdTaskListView *view,
                                       gboolean         show_list_name)
{
  g_return_if_fail (GTD_IS_TASK_LIST_VIEW (view));

  if (view->priv->show_list_name != show_list_name)
    {
      GList *children;
      GList *l;

      view->priv->show_list_name = show_list_name;

      /* update current children */
      children = gtk_container_get_children (GTK_CONTAINER (view->priv->listbox));

      for (l = children; l != NULL; l = l->next)
        gtd_task_row_set_list_name_visible (l->data, show_list_name);

      g_list_free (children);

      g_object_notify (G_OBJECT (view), "show-list-name");
    }
}

/**
 * gtd_task_list_view_get_show_completed:
 * @view: a #GtdTaskListView
 *
 * Returns %TRUE if completed tasks are visible, %FALSE otherwise.
 *
 * Returns: %TRUE if completed tasks are visible, %FALSE if they are hidden
 */
gboolean
gtd_task_list_view_get_show_completed (GtdTaskListView *view)
{
  g_return_val_if_fail (GTD_IS_TASK_LIST_VIEW (view), FALSE);

  return view->priv->show_completed;
}

/**
 * gtd_task_list_view_set_show_completed:
 * @view: a #GtdTaskListView
 * @show_completed: %TRUE to show completed tasks, %FALSE to hide them
 *
 * Sets the ::show-completed property to @show_completed.
 */
void
gtd_task_list_view_set_show_completed (GtdTaskListView *view,
                                       gboolean         show_completed)
{
  GtdTaskListViewPrivate *priv = view->priv;

  g_return_if_fail (GTD_IS_TASK_LIST_VIEW (view));

  if (priv->show_completed != show_completed)
    {

      priv->show_completed = show_completed;

      gtk_image_set_from_icon_name (view->priv->done_image,
                                    show_completed ? "zoom-out-symbolic" : "zoom-in-symbolic",
                                    GTK_ICON_SIZE_BUTTON);


      /* insert or remove list rows */
      if (show_completed)
        {
          GList *list_of_tasks;
          GList *l;

          list_of_tasks = gtd_task_list_view_get_list (view);

          for (l = list_of_tasks; l != NULL; l = l->next)
            {
              GtkWidget *new_row;

              if (!gtd_task_get_complete (l->data))
                continue;

              new_row = gtd_task_row_new (l->data);

              gtd_task_row_set_list_name_visible (GTD_TASK_ROW (new_row), priv->show_list_name);


              gtk_list_box_insert (priv->listbox,
                                   new_row,
                                   0);

              gtd_task_row_reveal (GTD_TASK_ROW (new_row));
            }

            g_list_free (list_of_tasks);
        }
      else
        {
          GList *children;
          GList *l;

          children = gtk_container_get_children (GTK_CONTAINER (priv->listbox));

          for (l = children; l != NULL; l = l->next)
            {
              if (!gtd_task_row_get_new_task_mode (l->data) &&
                  gtd_task_get_complete (gtd_task_row_get_task (l->data)))
                {
                  gtk_widget_destroy (l->data);
                }
            }

          g_list_free (children);
        }

      /* Check if it should show the empty state */
      gtd_task_list_view__update_empty_state (view);

      g_object_notify (G_OBJECT (view), "show-completed");
    }
}

/**
 * gtd_task_list_view_set_header_func:
 * @view: a #GtdTaskListView
 * @func: (closure user_data) (scope call) (nullable): the header function
 * @user_data: data passed to @func
 *
 * Sets @func as the header function of @view. You can safely call
 * %gtk_list_box_row_set_header from within @func.
 *
 * Do not unref nor free any of the passed data.
 */
void
gtd_task_list_view_set_header_func (GtdTaskListView           *view,
                                    GtdTaskListViewHeaderFunc  func,
                                    gpointer                   user_data)
{
  GtdTaskListViewPrivate *priv;

  g_return_if_fail (GTD_IS_TASK_LIST_VIEW (view));

  priv = view->priv;

  if (func)
    {
      priv->header_func = func;
      priv->header_user_data = user_data;

      gtk_list_box_set_header_func (priv->listbox,
                                    (GtkListBoxUpdateHeaderFunc) internal_header_func,
                                    view,
                                    NULL);
    }
  else
    {
      priv->header_func = NULL;
      priv->header_user_data = NULL;

      gtk_list_box_set_header_func (priv->listbox,
                                    NULL,
                                    NULL,
                                    NULL);
    }
}

/**
 * gtd_task_list_view_set_sort_func:
 * @view: a #GtdTaskListView
 * @func: (closure user_data) (scope call) (nullable): the sort function
 * @user_data: data passed to @func
 *
 * Sets @func as the sorting function of @view.
 *
 * Do not unref nor free any of the passed data.
 */
void
gtd_task_list_view_set_sort_func (GtdTaskListView         *view,
                                  GtdTaskListViewSortFunc  func,
                                  gpointer                 user_data)
{
  GtdTaskListViewPrivate *priv;

  g_return_if_fail (GTD_IS_TASK_LIST_VIEW (view));

  priv = gtd_task_list_view_get_instance_private (view);

  if (func)
    {
      priv->sort_func = func;
      priv->header_user_data = user_data;

      gtk_list_box_set_sort_func (priv->listbox,
                                  (GtkListBoxSortFunc) internal_sort_func,
                                  view,
                                  NULL);
    }
  else
    {
      priv->sort_func = NULL;
      priv->sort_user_data = NULL;

      gtk_list_box_set_sort_func (priv->listbox,
                                  (GtkListBoxSortFunc) gtd_task_list_view__listbox_sort_func,
                                  NULL,
                                  NULL);
    }
}

/**
 * gtd_task_list_view_get_default_date:
 * @self: a #GtdTaskListView
 *
 * Retrieves the current default date which new tasks are set to.
 *
 * Returns: (nullable): a #GDateTime, or %NULL
 */
GDateTime*
gtd_task_list_view_get_default_date (GtdTaskListView *self)
{
  GtdTaskListViewPrivate *priv;

  g_return_val_if_fail (GTD_IS_TASK_LIST_VIEW (self), NULL);

  priv = gtd_task_list_view_get_instance_private (self);

  return priv->default_date;
}

/**
 * gtd_task_list_view_set_default_date:
 * @self: a #GtdTaskListView
 * @default_date: (nullable): the default_date, or %NULL
 *
 * Sets the current default date.
 */
void
gtd_task_list_view_set_default_date   (GtdTaskListView *self,
                                       GDateTime       *default_date)
{
  GtdTaskListViewPrivate *priv;

  g_return_if_fail (GTD_IS_TASK_LIST_VIEW (self));

  priv = gtd_task_list_view_get_instance_private (self);

  if (priv->default_date != default_date)
    {
      g_clear_pointer (&priv->default_date, g_date_time_unref);
      priv->default_date = default_date ? g_date_time_ref (default_date) : NULL;
    }
}

/**
 * gtd_task_list_view_get_color:
 * @self: a #GtdTaskListView
 *
 * Retrieves the custom color of @self.
 *
 * Returns: (nullable): a #GdkRGBA, or %NULL if none is set.
 */
GdkRGBA*
gtd_task_list_view_get_color (GtdTaskListView *self)
{
  GtdTaskListViewPrivate *priv;

  g_return_val_if_fail (GTD_IS_TASK_LIST_VIEW (self), NULL);

  priv = gtd_task_list_view_get_instance_private (self);

  return priv->color;
}

/**
 * gtd_task_list_view_set_color:
 * @self: a #GtdTaskListView
 * @color: (nullable): a #GdkRGBA
 *
 * Sets the custom color of @self to @color. If a custom color is set,
 * the tasklist's color is ignored. Passing %NULL makes the tasklist's
 * color apply again.
 */
void
gtd_task_list_view_set_color (GtdTaskListView *self,
                              GdkRGBA         *color)
{
  GtdTaskListViewPrivate *priv;

  g_return_if_fail (GTD_IS_TASK_LIST_VIEW (self));

  priv = gtd_task_list_view_get_instance_private (self);

  if (priv->color != color ||
      (color && priv->color && !gdk_rgba_equal (color, priv->color)))
    {
      g_clear_pointer (&priv->color, gdk_rgba_free);
      priv->color = gdk_rgba_copy (color);

      gtd_task_list_view__color_changed (self);

      g_object_notify (G_OBJECT (self), "color");
    }
}
