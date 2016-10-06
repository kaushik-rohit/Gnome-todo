include $(top_srcdir)/common.am

noinst_LTLIBRARIES = libeds.la

libeds_la_SOURCES = \
	gtd-panel-today.c \
	gtd-panel-today.h \
	gtd-panel-scheduled.c \
	gtd-panel-scheduled.h \
	gtd-plugin-eds.c \
	gtd-plugin-eds.h \
	gtd-provider-eds.c \
	gtd-provider-eds.h \
	gtd-provider-goa.c \
	gtd-provider-goa.h \
	gtd-provider-local.c \
	gtd-provider-local.h \
	gtd-task-list-eds.c \
	gtd-task-list-eds.h

libeds_la_CFLAGS = \
	$(GNOME_TODO_PLUGIN_CFLAGS) \
	$(GNOME_TODO_PLUGIN_WARN_CFLAGS) \
	-DGOA_API_IS_SUBJECT_TO_CHANGE \
	-DG_LOG_DOMAIN='"EDS"'

EXTRA_DIST = \
	eds.plugin.in

