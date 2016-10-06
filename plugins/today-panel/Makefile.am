include $(top_srcdir)/common.am

NULL=

noinst_LTLIBRARIES = libtodaypanel.la

libtodaypanel_la_SOURCES = \
	gtd-panel-today.c \
	gtd-panel-today.h \
	gtd-plugin-today-panel.c \
	gtd-plugin-today-panel.h \
	$(NULL)

libtodaypanel_la_CFLAGS = \
	$(GNOME_TODO_PLUGIN_CFLAGS) \
	$(GNOME_TODO_PLUGIN_WARN_CFLAGS)

EXTRA_DIST = \
	today-panel.plugin.in
