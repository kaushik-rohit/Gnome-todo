include $(top_srcdir)/common.am

NULL=

noinst_LTLIBRARIES = libdarktheme.la

libdarktheme_la_SOURCES = \
	gtd-plugin-dark-theme.c \
	gtd-plugin-dark-theme.h \
	$(NULL)

libdarktheme_la_CFLAGS = \
	$(GNOME_TODO_PLUGIN_CFLAGS) \
	$(GNOME_TODO_PLUGIN_WARN_CFLAGS)

EXTRA_DIST = \
	dark-theme.plugin.in
