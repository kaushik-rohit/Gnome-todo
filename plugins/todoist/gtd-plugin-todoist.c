/* gtd-plugin-todoist.c
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

#define G_LOG_DOMAIN "Todoist"

#include "gtd-plugin-todoist.h"
#include "gtd-provider-todoist.h"
#include "gtd-todoist-preferences-panel.h"

#include <glib/gi18n.h>
#include <glib-object.h>

/**
 * The #GtdPluginTodoist is a class that loads Todoist
 * provider of GNOME To Do.
 */

struct _GtdPluginTodoist
{
  PeasExtensionBase   parent;

  GtkWidget          *preferences;

  /* Providers */
  GList              *providers;
};

enum
{
  PROP_0,
  PROP_PREFERENCES_PANEL,
  LAST_PROP
};

static void          gtd_activatable_iface_init                  (GtdActivatableInterface  *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (GtdPluginTodoist, gtd_plugin_todoist, PEAS_TYPE_EXTENSION_BASE,
                                0,
                                G_IMPLEMENT_INTERFACE_DYNAMIC (GTD_TYPE_ACTIVATABLE,
                                                               gtd_activatable_iface_init))

/*
 * GtdActivatable interface implementation
 */
static void
gtd_plugin_todoist_activate (GtdActivatable *activatable)
{
  ;
}

static void
gtd_plugin_todoist_deactivate (GtdActivatable *activatable)
{
  ;
}

static GList*
gtd_plugin_todoist_get_header_widgets (GtdActivatable *activatable)
{
  return NULL;
}

static GtkWidget*
gtd_plugin_todoist_get_preferences_panel (GtdActivatable *activatable)
{
  GtdPluginTodoist *self = GTD_PLUGIN_TODOIST (activatable);

  return self->preferences;

}

static GList*
gtd_plugin_todoist_get_panels (GtdActivatable *activatable)
{
  return NULL;
}

static GList*
gtd_plugin_todoist_get_providers (GtdActivatable *activatable)
{
  GtdPluginTodoist *plugin = GTD_PLUGIN_TODOIST (activatable);

  return plugin->providers;
}

static void
gtd_plugin_todoist_account_added (GtdTodoistPreferencesPanel *panel,
                                  GoaObject                  *account_object,
                                  GtdPluginTodoist           *self)
{
  GtdProviderTodoist *provider;
  GoaAccount *goa_account;
  const gchar *provider_name;

  goa_account = goa_object_get_account (account_object);
  provider_name = goa_account_get_provider_name (goa_account);

  if (g_strcmp0 (provider_name, "Todoist") != 0)
    return;

  provider = gtd_provider_todoist_new (account_object);

  self->providers = g_list_append (self->providers, provider);

  g_signal_emit_by_name (self, "provider-added", provider);
}

static void
gtd_plugin_todoist_account_removed (GtdTodoistPreferencesPanel *panel,
                                    GoaObject                  *account_object,
                                    GtdPluginTodoist           *self)
{
  GoaAccount *goa_account;
  const gchar *provider_name;
  GList *l;

  goa_account = goa_object_get_account (account_object);
  provider_name = goa_account_get_provider_name (goa_account);
  l = NULL;

  if (g_strcmp0 (provider_name, "Todoist") != 0)
    return;

  for (l = self->providers; l != NULL; l = l->next)
    {
      GoaObject *object;

      object = gtd_provider_todoist_get_goa_object (l->data);

      if (object == account_object)
        {
          self->providers = g_list_remove (self->providers, l->data);

          g_signal_emit_by_name (self, "provider-removed", l->data);

          break;
        }
    }
}

static void
gtd_plugin_todoist_account_changed (GtdTodoistPreferencesPanel *panel,
                                    GoaObject                  *account_object,
                                    GtdPluginTodoist           *self)
{
  GoaAccount *goa_account;
  const gchar *provider_name;

  goa_account = goa_object_get_account (account_object);
  provider_name = goa_account_get_provider_name (goa_account);

  if (g_strcmp0 (provider_name, "Todoist") != 0)
    return;
}

static void
goa_client_ready (GObject           *source,
                  GAsyncResult      *res,
                  GtdPluginTodoist  *self)
{
  GoaClient *client;
  GList *accounts;
  GList *l;

  accounts = NULL;
  l = NULL;

  client = goa_client_new_finish (res, NULL);

  accounts = goa_client_get_accounts (client);

  for (l = accounts; l != NULL; l = l->next)
    {
      GoaAccount *account;
      const gchar *provider_type;

      account = goa_object_get_account (l->data);
      provider_type = goa_account_get_provider_type (account);

      if (g_strcmp0 (provider_type, "todoist") == 0)
        gtd_plugin_todoist_account_added (self->preferences, l->data, self);

      g_object_unref (account);
    }

  /* Connect signals */
  g_signal_connect (client, "account-added", G_CALLBACK (gtd_plugin_todoist_account_added), self);
  g_signal_connect (client, "account-removed", G_CALLBACK (gtd_plugin_todoist_account_removed), self);
  g_signal_connect (client, "account-changed", G_CALLBACK (gtd_plugin_todoist_account_changed), self);

  gtd_todoist_preferences_panel_set_client (GTD_TODOIST_PREFERENCES_PANEL (self->preferences), client);

  g_list_free_full (accounts,  g_object_unref);
}

static void
gtd_activatable_iface_init (GtdActivatableInterface *iface)
{
  iface->activate = gtd_plugin_todoist_activate;
  iface->deactivate = gtd_plugin_todoist_deactivate;
  iface->get_header_widgets = gtd_plugin_todoist_get_header_widgets;
  iface->get_preferences_panel = gtd_plugin_todoist_get_preferences_panel;
  iface->get_panels = gtd_plugin_todoist_get_panels;
  iface->get_providers = gtd_plugin_todoist_get_providers;
}

/*
 * Init
 */

static void
gtd_plugin_todoist_finalize (GObject *object)
{
  GtdPluginTodoist *self = (GtdPluginTodoist *) object;

  g_list_free_full (self->providers, g_object_unref);
  self->providers = NULL;

  G_OBJECT_CLASS (gtd_plugin_todoist_parent_class)->finalize (object);
}

static void
gtd_plugin_todoist_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GtdPluginTodoist *self = GTD_PLUGIN_TODOIST (object);
  switch (prop_id)
    {
    case PROP_PREFERENCES_PANEL:
      g_value_set_object (value, self->preferences);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtd_plugin_todoist_class_init (GtdPluginTodoistClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gtd_plugin_todoist_finalize;
  object_class->get_property = gtd_plugin_todoist_get_property;

  g_object_class_override_property (object_class,
                                    PROP_PREFERENCES_PANEL,
                                    "preferences-panel");
}

static void
gtd_plugin_todoist_init (GtdPluginTodoist *self)
{
  self->preferences = GTK_WIDGET (gtd_todoist_preferences_panel_new ());

  goa_client_new (NULL, (GAsyncReadyCallback) goa_client_ready, self);
}

/* Empty class_finalize method */
static void
gtd_plugin_todoist_class_finalize (GtdPluginTodoistClass *klass)
{
}

G_MODULE_EXPORT void
gtd_plugin_todoist_register_types (PeasObjectModule *module)
{
  gtd_plugin_todoist_register_type (G_TYPE_MODULE (module));

  peas_object_module_register_extension_type (module,
                                              GTD_TYPE_ACTIVATABLE,
                                              GTD_TYPE_PLUGIN_TODOIST);
}
