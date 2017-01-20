gsettings_SCHEMAS = org.gnome.todo.txt.gschema.xml

@GSETTINGS_RULES@

include $(top_srcdir)/common.am

noinst_LTLIBRARIES = libtodotxt.la

libtodotxt_la_SOURCES = \
	gtd-plugin-todo-txt.c \
	gtd-plugin-todo-txt.h \
	gtd-provider-todo-txt.c \
	gtd-provider-todo-txt.h \
	gtd-todo-txt-parser.c \
	gtd-todo-txt-parser.h

libtodotxt_la_CFLAGS = \
	$(GNOME_TODO_PLUGIN_CFLAGS) \
	$(GNOME_TODO_PLUGIN_WARN_CFLAGS)

EXTRA_DIST = \
	$(gsettings_SCHEMAS) \
	todo-txt.plugin.in

