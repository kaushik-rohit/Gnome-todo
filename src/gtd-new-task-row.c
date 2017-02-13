/* gtd-new-task-row.c
 *
 * Copyright (C) 2017 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
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

#include "gtd-new-task-row.h"
#include "gtd-task.h"

struct _GtdNewTaskRow
{
  GtkListBoxRow       parent;

  GtkEntry           *entry;
  GtkStack           *stack;
};

G_DEFINE_TYPE (GtdNewTaskRow, gtd_new_task_row, GTK_TYPE_LIST_BOX_ROW)

enum
{
  ENTER,
  EXIT,
  CREATE_TASK,
  NUM_SIGNALS
};

enum
{
  PROP_0,
  N_PROPS
};

static guint          signals [NUM_SIGNALS] = { 0, };

/*
 * Auxiliary methods
 */

static gboolean
gtd_new_task_row_focus_in_event (GtkWidget     *widget,
                                 GdkEventFocus *event)
{
  GtdNewTaskRow *self = GTD_NEW_TASK_ROW (widget);

  gtd_new_task_row_set_active (self, TRUE);

  return GDK_EVENT_PROPAGATE;
}

/*
 * Callbacks
 */

static void
entry_activated_cb (GtdNewTaskRow *self)
{
  GtdTask *new_task;

  /* Cannot create empty tasks */
  if (gtk_entry_get_text_length (self->entry) == 0)
    return;

  new_task = gtd_task_new (NULL);
  gtd_task_set_title (new_task, gtk_entry_get_text (self->entry));
  gtd_task_save (new_task);

  g_signal_emit (self, signals[CREATE_TASK], 0, new_task);

  gtk_entry_set_text (self->entry, "");
}

static void
gtd_new_task_row_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
gtd_new_task_row_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
gtd_new_task_row_class_init (GtdNewTaskRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = gtd_new_task_row_get_property;
  object_class->set_property = gtd_new_task_row_set_property;

  widget_class->focus_in_event = gtd_new_task_row_focus_in_event;

  /**
   * GtdNewTaskRow::enter:
   *
   * Emitted when the row is focused and in the editing state.
   */
  signals[ENTER] = g_signal_new ("enter",
                                 GTD_TYPE_NEW_TASK_ROW,
                                 G_SIGNAL_RUN_LAST,
                                 0,
                                 NULL,
                                 NULL,
                                 NULL,
                                 G_TYPE_NONE,
                                 0);

  /**
   * GtdNewTaskRow::exit:
   *
   * Emitted when the row is unfocused and leaves the editing state.
   */
  signals[EXIT] = g_signal_new ("exit",
                                GTD_TYPE_NEW_TASK_ROW,
                                G_SIGNAL_RUN_LAST,
                                0,
                                NULL,
                                NULL,
                                NULL,
                                G_TYPE_NONE,
                                0);

  /**
   * GtdNewTaskRow::create-task:
   *
   * Emitted when the row wants the parent widget to create a new task.
   */
  signals[CREATE_TASK] = g_signal_new ("create-task",
                                       GTD_TYPE_NEW_TASK_ROW,
                                       G_SIGNAL_RUN_LAST,
                                       0,
                                       NULL,
                                       NULL,
                                       NULL,
                                       G_TYPE_NONE,
                                       1,
                                       GTD_TYPE_TASK);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/todo/ui/new-task-row.ui");

  gtk_widget_class_bind_template_child (widget_class, GtdNewTaskRow, entry);
  gtk_widget_class_bind_template_child (widget_class, GtdNewTaskRow, stack);

  gtk_widget_class_bind_template_callback (widget_class, entry_activated_cb);

  gtk_widget_class_set_css_name (widget_class, "taskrow");
}

static void
gtd_new_task_row_init (GtdNewTaskRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

GtkWidget*
gtd_new_task_row_new (void)
{
  return g_object_new (GTD_TYPE_NEW_TASK_ROW, NULL);
}

gboolean
gtd_new_task_row_get_active (GtdNewTaskRow *self)
{
  g_return_val_if_fail (GTD_IS_NEW_TASK_ROW (self), FALSE);

  return g_str_equal (gtk_stack_get_visible_child_name (self->stack), "entry");
}

void
gtd_new_task_row_set_active (GtdNewTaskRow *self,
                             gboolean       active)
{
  g_return_if_fail (GTD_IS_NEW_TASK_ROW (self));

  if (active)
    {
      gtk_stack_set_visible_child_name (self->stack, "entry");
      gtk_widget_grab_focus (GTK_WIDGET (self->entry));
      g_signal_emit (self, signals[ENTER], 0);
    }
  else
    {
      gtk_stack_set_visible_child_name (self->stack, "label");
      gtk_widget_grab_focus (GTK_WIDGET (self->entry));
    }
}
