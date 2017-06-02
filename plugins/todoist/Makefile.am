include $(top_srcdir)/common.am

noinst_LTLIBRARIES = libtodoist.la

libtodoist_la_SOURCES = \
	gtd-plugin-todoist.c \
	gtd-plugin-todoist.h \
	gtd-provider-todoist.c \
	gtd-provider-todoist.h \
	gtd-todoist-preferences-panel.c \
	gtd-todoist_preferences_panel.h

libtodoist_la_CFLAGS = \
	$(GNOME_TODO_PLUGIN_CFLAGS) \
	$(GNOME_TODO_PLUGIN_WARN_CFLAGS)
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\"

EXTRA_DIST = \
	todoist.plugin.in

