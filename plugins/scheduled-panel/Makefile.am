include $(top_srcdir)/common.am

NULL=

noinst_LTLIBRARIES = libscheduledpanel.la

libscheduledpanel_la_SOURCES = \
	gtd-panel-scheduled.c \
	gtd-panel-scheduled.h \
	gtd-plugin-scheduled-panel.c \
	gtd-plugin-scheduled-panel.h \
	$(NULL)

libscheduledpanel_la_CFLAGS = \
	$(GNOME_TODO_PLUGIN_CFLAGS) \
	$(GNOME_TODO_PLUGIN_WARN_CFLAGS)

EXTRA_DIST = \
	scheduled-panel.plugin.in
