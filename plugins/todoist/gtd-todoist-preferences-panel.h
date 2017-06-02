/* gtd-todoist-preferences-panel.h
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

#define GOA_API_IS_SUBJECT_TO_CHANGE

#ifndef GTD_TODOIST_PREFERENCES_PANEL_H
#define GTD_TODOIST_PREFERENCES_PANEL_H

#include <glib.h>
#include <gnome-todo.h>
#include <goa/goa.h>

G_BEGIN_DECLS

#define GTD_TYPE_TODOIST_PREFERENCES_PANEL (gtd_todoist_preferences_panel_get_type())

G_DECLARE_FINAL_TYPE (GtdTodoistPreferencesPanel, gtd_todoist_preferences_panel, GTD, TODOIST_PREFERENCES_PANEL, GtkStack)

GtdTodoistPreferencesPanel*   gtd_todoist_preferences_panel_new           (void);

void                          gtd_todoist_preferences_panel_set_client    (GtdTodoistPreferencesPanel *self,
                                                                           GoaClient                  *client);
G_END_DECLS

#endif /* GTD_TODOIST_PREFERENCES_PANEL_H */
