/* gtd-task.h
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

#ifndef GTD_TASK_H
#define GTD_TASK_H

#include "gtd-object.h"

#include <glib-object.h>
#include <libecal/libecal.h>

G_BEGIN_DECLS

#define GTD_TYPE_TASK (gtd_task_get_type())

G_DECLARE_DERIVABLE_TYPE (GtdTask, gtd_task, GTD, TASK, GtdObject)

struct _GtdTaskClass
{
  GtdObjectClass parent;

  /*< signals >*/
  void          (*subtask_added)                      (GtdTask              *self,
                                                       GtdTask              *subtask);

  void          (*subtask_removed)                    (GtdTask              *self,
                                                       GtdTask              *subtask);

  gpointer       padding[8];
};

GtdTask*            gtd_task_new                      (ECalComponent        *component);

gboolean            gtd_task_get_complete             (GtdTask              *task);

void                gtd_task_set_complete             (GtdTask              *task,
                                                       gboolean              complete);

ECalComponent*      gtd_task_get_component            (GtdTask              *task);

GDateTime*          gtd_task_get_creation_date        (GtdTask              *task);

const gchar*        gtd_task_get_description          (GtdTask              *task);

void                gtd_task_set_description          (GtdTask              *task,
                                                       const gchar          *description);

GDateTime*          gtd_task_get_due_date             (GtdTask              *task);

void                gtd_task_set_due_date             (GtdTask              *task,
                                                       GDateTime            *dt);

GtdTaskList*        gtd_task_get_list                 (GtdTask              *task);

void                gtd_task_set_list                 (GtdTask              *task,
                                                       GtdTaskList          *list);

gint                gtd_task_get_priority             (GtdTask              *task);

void                gtd_task_set_priority             (GtdTask              *task,
                                                       gint                  priority);

const gchar*        gtd_task_get_title                (GtdTask              *task);

void                gtd_task_set_title                (GtdTask              *task,
                                                       const gchar          *title);

void                gtd_task_abort                    (GtdTask              *task);

void                gtd_task_save                     (GtdTask              *task);

gint                gtd_task_compare                  (GtdTask              *t1,
                                                       GtdTask              *t2);

GtdTask*            gtd_task_get_parent               (GtdTask              *self);

GList*              gtd_task_get_substasks            (GtdTask              *self);

void                gtd_task_add_subtask              (GtdTask              *self,
                                                       GtdTask              *subtask);

void                gtd_task_remove_subtask           (GtdTask              *self,
                                                       GtdTask              *subtask);

G_END_DECLS

#endif /* GTD_TASK_H */
