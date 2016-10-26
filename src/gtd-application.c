/* gtd-application.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gtd-application.h"
#include "gtd-initial-setup-window.h"
#include "gtd-manager.h"
#include "gtd-manager-protected.h"
#include "gtd-plugin-dialog.h"
#include "gtd-window.h"

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <girepository.h>
#include <glib/gi18n.h>

typedef struct
{
  GtkCssProvider *provider;
  GtdManager     *manager;

  GtkWidget      *window;
  GtkWidget      *plugin_dialog;
  GtkWidget      *initial_setup;
} GtdApplicationPrivate;

struct _GtdApplication
{
  GtkApplication         application;

  /*< private >*/
  GtdApplicationPrivate *priv;
};

static void           gtd_application_show_extensions             (GSimpleAction        *simple,
                                                                   GVariant             *parameter,
                                                                   gpointer              user_data);

static void           gtd_application_show_about                  (GSimpleAction        *simple,
                                                                   GVariant             *parameter,
                                                                   gpointer              user_data);

static void           gtd_application_quit                        (GSimpleAction        *simple,
                                                                   GVariant             *parameter,
                                                                   gpointer              user_data);

G_DEFINE_TYPE_WITH_PRIVATE (GtdApplication, gtd_application, GTK_TYPE_APPLICATION)

static const GActionEntry gtd_application_entries[] = {
  { "show-extensions",  gtd_application_show_extensions },
  { "about",  gtd_application_show_about },
  { "quit",   gtd_application_quit }
};

static void
gtd_application_show_extensions (GSimpleAction *simple,
                                 GVariant      *parameter,
                                 gpointer       user_data)
{
  GtdApplicationPrivate *priv = GTD_APPLICATION (user_data)->priv;

  gtk_widget_show (priv->plugin_dialog);
}

static void
gtd_application_show_about (GSimpleAction *simple,
                            GVariant      *parameter,
                            gpointer       user_data)
{
  GtdApplicationPrivate *priv = GTD_APPLICATION (user_data)->priv;
  char *copyright;
  GDateTime *date;
  int created_year = 2015;

  static const gchar *authors[] = {
    "Emmanuele Bassi <ebassi@gnome.org>",
    "Georges Basile Stavracas Neto <georges.stavracas@gmail.com>",
    "Isaque Galdino <igaldino@gmail.com>",
    "Patrick Griffis <tingping@tingping.se>",
    "Saiful B. Khan <saifulbkhan@gmail.com>",
    NULL
  };

  static const gchar *artists[] = {
    "Allan Day <allanpday@gmail.com>",
    "Jakub Steiner <jimmac@gmail.com>",
    NULL
  };

  date = g_date_time_new_now_local ();

  if (g_date_time_get_year (date) == created_year)
    {
      copyright = g_strdup_printf (_("Copyright \xC2\xA9 %d "
                                     "The To Do authors"), created_year);
    }
  else
    {
      copyright = g_strdup_printf (_("Copyright \xC2\xA9 %d\xE2\x80\x93%d "
                                     "The To Do authors"), created_year, g_date_time_get_year (date));
    }

  gtk_show_about_dialog (GTK_WINDOW (priv->window),
                         "program-name", _("To Do"),
                         "version", VERSION,
                         "copyright", copyright,
                         "license-type", GTK_LICENSE_GPL_3_0,
                         "authors", authors,
                         "artists", artists,
                         "logo-icon-name", "org.gnome.Todo",
                         "translator-credits", _("translator-credits"),
                         NULL);
  g_free (copyright);
  g_date_time_unref (date);
}

static void
gtd_application_quit (GSimpleAction *simple,
                      GVariant      *parameter,
                      gpointer       user_data)
{
  GtdApplicationPrivate *priv = GTD_APPLICATION (user_data)->priv;

  gtk_widget_destroy (priv->window);
}

GtdApplication *
gtd_application_new (void)
{
  g_set_application_name (_("To Do"));

  return g_object_new (GTD_TYPE_APPLICATION,
                       "application-id", "org.gnome.Todo",
                       "flags", G_APPLICATION_FLAGS_NONE,
                       "resource-base-path", "/org/gnome/todo",
                       NULL);
}

static void
run_window (GtdApplication *application)
{
  GtdApplicationPrivate *priv;

  g_return_if_fail (GTD_IS_APPLICATION (application));

  priv = application->priv;

  if (!priv->window)
    {
      priv->window = gtd_window_new (GTD_APPLICATION (application));

      gtk_window_set_transient_for (GTK_WINDOW (priv->plugin_dialog),
                                    GTK_WINDOW (priv->window));
    }

  gtk_widget_show (priv->window);
}

/*
static void
finish_initial_setup (GtdApplication *application)
{
  g_return_if_fail (GTD_IS_APPLICATION (application));

  run_window (application);

  gtd_manager_set_is_first_run (application->priv->manager, FALSE);

  g_clear_pointer (&application->priv->initial_setup, gtk_widget_destroy);
}

static void
run_initial_setup (GtdApplication *application)
{
  GtdApplicationPrivate *priv;

  g_return_if_fail (GTD_IS_APPLICATION (application));

  priv = application->priv;

  if (!priv->initial_setup)
    {
      priv->initial_setup = gtd_initial_setup_window_new (application);

      g_signal_connect (priv->initial_setup,
                        "cancel",
                        G_CALLBACK (gtk_widget_destroy),
                        application);

      g_signal_connect_swapped (priv->initial_setup,
                                "done",
                                G_CALLBACK (finish_initial_setup),
                                application);
    }

  gtk_widget_show (priv->initial_setup);
}
*/

static void
gtd_application_activate (GApplication *application)
{
  GtdApplicationPrivate *priv = GTD_APPLICATION (application)->priv;

  if (!priv->provider)
   {
     GError *error = NULL;
     GFile* css_file;

     priv->provider = gtk_css_provider_new ();
     gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                                GTK_STYLE_PROVIDER (priv->provider),
                                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 1);

     css_file = g_file_new_for_uri ("resource:///org/gnome/todo/theme/Adwaita.css");

     gtk_css_provider_load_from_file (priv->provider,
                                      css_file,
                                      &error);
     if (error != NULL)
       {
         g_warning ("%s: %s: %s",
                    G_STRFUNC,
                    _("Error loading CSS from resource"),
                    error->message);

         g_error_free (error);
       }
     else
       {
         g_object_unref (css_file);
       }
   }

  /* FIXME: the initial setup is disabled for the 3.18 release because
   * we can't create tasklists on GOA accounts.
   */
  run_window (GTD_APPLICATION (application));
}

static void
gtd_application_finalize (GObject *object)
{
  G_OBJECT_CLASS (gtd_application_parent_class)->finalize (object);
}

static void
gtd_application_startup (GApplication *application)
{
  GtdApplicationPrivate *priv = GTD_APPLICATION (application)->priv;

  /* add actions */
  g_action_map_add_action_entries (G_ACTION_MAP (application),
                                   gtd_application_entries,
                                   G_N_ELEMENTS (gtd_application_entries),
                                   application);

  G_APPLICATION_CLASS (gtd_application_parent_class)->startup (application);

  /* plugin dialog */
  priv->plugin_dialog = gtd_plugin_dialog_new ();

  /* manager */
  priv->manager = gtd_manager_get_default ();
  gtd_manager_load_plugins (priv->manager);
}

static gboolean
gtd_application_local_command_line (GApplication   *application,
                                    gchar        ***arguments,
                                    gint           *exit_status)
{
  g_application_add_option_group (application, g_irepository_get_option_group());

  return G_APPLICATION_CLASS (gtd_application_parent_class)->local_command_line (application,
                                                                                 arguments,
                                                                                 exit_status);
}

static void
gtd_application_class_init (GtdApplicationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GApplicationClass *application_class = G_APPLICATION_CLASS (klass);

  object_class->finalize = gtd_application_finalize;

  application_class->activate = gtd_application_activate;
  application_class->startup = gtd_application_startup;
  application_class->local_command_line = gtd_application_local_command_line;
}

static void
gtd_application_init (GtdApplication *self)
{
  GtdApplicationPrivate *priv = gtd_application_get_instance_private (self);

  self->priv = priv;
}

GtdManager*
gtd_application_get_manager (GtdApplication *app)
{
  g_return_val_if_fail (GTD_IS_APPLICATION (app), NULL);

  return app->priv->manager;
}
