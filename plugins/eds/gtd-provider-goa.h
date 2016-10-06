/* gtd-provider-goa.h
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

#ifndef GTD_PROVIDER_GOA_H
#define GTD_PROVIDER_GOA_H

#define GOA_API_IS_SUBJECT_TO_CHANGE 1

#include "gtd-provider-eds.h"

#include <glib.h>
#include <gnome-todo.h>
#include <goa/goa.h>

G_BEGIN_DECLS

#define GTD_TYPE_PROVIDER_GOA (gtd_provider_goa_get_type())

G_DECLARE_FINAL_TYPE (GtdProviderGoa, gtd_provider_goa, GTD, PROVIDER_GOA, GtdProviderEds)

GtdProviderGoa*      gtd_provider_goa_new                        (ESourceRegistry    *registry,
                                                                  GoaAccount         *account);

GoaAccount*          gtd_provider_goa_get_account                (GtdProviderGoa     *provider);

G_END_DECLS

#endif /* GTD_PROVIDER_GOA_H */
