/* gtd-storage.h
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

#ifndef GTD_PROVIDER_H
#define GTD_PROVIDER_H

#include "gtd-object.h"
#include "gtd-types.h"

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTD_TYPE_PROVIDER (gtd_provider_get_type ())

G_DECLARE_INTERFACE (GtdProvider, gtd_provider, GTD, PROVIDER, GtdObject)

struct _GtdProviderInterface
{
  GTypeInterface parent;

  /* Information */
  const gchar*       (*get_id)                                   (GtdProvider        *provider);

  const gchar*       (*get_name)                                 (GtdProvider        *provider);

  const gchar*       (*get_description)                          (GtdProvider        *provider);

  gboolean           (*get_enabled)                              (GtdProvider        *provider);

  /* Customs */
  GIcon*             (*get_icon)                                 (GtdProvider        *provider);

  const GtkWidget*   (*get_edit_panel)                           (GtdProvider        *provider);

  /* Tasks */
  void               (*create_task)                              (GtdProvider        *provider,
                                                                  GtdTask            *task);

  void               (*update_task)                              (GtdProvider        *provider,
                                                                  GtdTask            *task);

  void               (*remove_task)                              (GtdProvider        *provider,
                                                                  GtdTask            *task);

  /* Task lists */
  void               (*create_task_list)                         (GtdProvider        *provider,
                                                                  GtdTaskList        *list);

  void               (*update_task_list)                         (GtdProvider        *provider,
                                                                  GtdTaskList        *list);

  void               (*remove_task_list)                         (GtdProvider        *provider,
                                                                  GtdTaskList        *list);

  GList*             (*get_task_lists)                           (GtdProvider        *provider);

  GtdTaskList*       (*get_default_task_list)                    (GtdProvider        *provider);
};

const gchar*         gtd_provider_get_id                         (GtdProvider        *provider);

const gchar*         gtd_provider_get_name                       (GtdProvider        *provider);

const gchar*         gtd_provider_get_description                (GtdProvider        *provider);

gboolean             gtd_provider_get_enabled                    (GtdProvider        *provider);

GIcon*               gtd_provider_get_icon                       (GtdProvider        *provider);

const GtkWidget*     gtd_provider_get_edit_panel                 (GtdProvider        *provider);

void                 gtd_provider_create_task                    (GtdProvider        *provider,
                                                                  GtdTask            *task);

void                 gtd_provider_update_task                    (GtdProvider        *provider,
                                                                  GtdTask            *task);

void                 gtd_provider_remove_task                    (GtdProvider        *provider,
                                                                  GtdTask            *task);

void                 gtd_provider_create_task_list               (GtdProvider        *provider,
                                                                  GtdTaskList        *list);

void                 gtd_provider_update_task_list               (GtdProvider        *provider,
                                                                  GtdTaskList        *list);

void                 gtd_provider_remove_task_list               (GtdProvider        *provider,
                                                                  GtdTaskList        *list);

GList*               gtd_provider_get_task_lists                 (GtdProvider        *provider);

GtdTaskList*         gtd_provider_get_default_task_list          (GtdProvider        *provider);

G_END_DECLS

#endif /* GTD_PROVIDER_H */
