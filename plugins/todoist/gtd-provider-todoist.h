/* gtd-provider-todoist.h
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

#ifndef GTD_PROVIDER_TODOIST_H
#define GTD_PROVIDER_TODOIST_H

#define GOA_API_IS_SUBJECT_TO_CHANGE

#include <glib.h>
#include <gnome-todo.h>
#include <goa/goa.h>

G_BEGIN_DECLS

#define GTD_TYPE_PROVIDER_TODOIST (gtd_provider_todoist_get_type())

G_DECLARE_FINAL_TYPE (GtdProviderTodoist, gtd_provider_todoist, GTD, PROVIDER_TODOIST, GtdObject)

GtdProviderTodoist*    gtd_provider_todoist_new                     (GoaObject *account_object);

GoaObject*             gtd_provider_todoist_get_goa_object          (GtdProviderTodoist  *self);

G_END_DECLS

#endif /* GTD_PROVIDER_TODOIST_H */
