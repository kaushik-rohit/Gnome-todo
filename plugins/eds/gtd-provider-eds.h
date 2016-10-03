/* gtd-provider-eds.h
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

#ifndef GTD_PROVIDER_EDS_H
#define GTD_PROVIDER_EDS_H

#include <glib.h>
#include <gnome-todo.h>
#include <libecal/libecal.h>
#include <libedataserverui/libedataserverui.h>

G_BEGIN_DECLS

#define GTD_TYPE_PROVIDER_EDS (gtd_provider_eds_get_type())

G_DECLARE_DERIVABLE_TYPE (GtdProviderEds, gtd_provider_eds, GTD, PROVIDER_EDS, GtdObject)

struct _GtdProviderEdsClass
{
  GtdObjectClass parent;

  gboolean           (*should_load_source)                       (GtdProviderEds     *provider,
                                                                  ESource            *source);
};

GtdProviderEds*      gtd_provider_eds_new                        (ESourceRegistry    *registry);

ESourceRegistry*     gtd_provider_eds_get_registry               (GtdProviderEds     *local);

void                 gtd_provider_eds_create_task                (GtdProviderEds     *provider,
                                                                  GtdTask            *task);

void                 gtd_provider_eds_update_task                (GtdProviderEds     *provider,
                                                                  GtdTask            *task);

void                 gtd_provider_eds_remove_task                (GtdProviderEds     *provider,
                                                                  GtdTask            *task);

void                 gtd_provider_eds_create_task_list           (GtdProviderEds     *provider,
                                                                  GtdTaskList        *list);

void                 gtd_provider_eds_update_task_list           (GtdProviderEds     *provider,
                                                                  GtdTaskList        *list);

void                 gtd_provider_eds_remove_task_list           (GtdProviderEds     *provider,
                                                                  GtdTaskList        *list);

GList*               gtd_provider_eds_get_task_lists             (GtdProviderEds     *provider);

GtdTaskList*         gtd_provider_eds_get_default_task_list      (GtdProviderEds     *provider);

G_END_DECLS

#endif /* GTD_PROVIDER_EDS_H */
