gsettings_SCHEMAS = org.gnome.todo.background.gschema.xml

@GSETTINGS_RULES@

include $(top_srcdir)/common.am

noinst_LTLIBRARIES = libbackground.la

libbackground_la_SOURCES = \
	gtd-plugin-background.c \
	gtd-plugin-background.h

libbackground_la_CFLAGS = \
	$(GNOME_TODO_PLUGIN_CFLAGS) \
	$(GNOME_TODO_PLUGIN_WARN_CFLAGS) \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\"

EXTRA_DIST = \
	$(gsettings_SCHEMAS) \
	background.plugin.in
