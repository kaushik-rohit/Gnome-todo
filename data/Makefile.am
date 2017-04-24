SUBDIRS = icons

pkgconfigdir = ${libdir}/pkgconfig
pkgconfig_DATA = gnome-todo.pc

desktopdir = $(datadir)/applications
desktop_in_files = org.gnome.Todo.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

autostartdir = $(pkgdatadir)
autostart_DATA = org.gnome.Todo.Autostart.desktop

%.desktop: %.desktop.in
	$(AM_V_GEN) $(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

appdatadir = $(datadir)/appdata
appdata_DATA = $(appdata_in_files:.xml.in=.xml)
appdata_in_files = $(srcdir)/appdata/org.gnome.Todo.appdata.xml.in

%.xml: %.xml.in
	$(AM_V_GEN) $(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@

gsettings_ENUM_NAMESPACE = org.gnome.todo
gsettings_ENUM_FILES = \
	$(top_srcdir)/src/gtd-enums.h

gsettings_SCHEMAS = org.gnome.todo.gschema.xml

@GSETTINGS_RULES@

%.service: %.service.in
	$(AM_V_GEN)$(SED) -e "s|[@]bindir[@]|$(bindir)|" $< > $@

servicedir = $(datadir)/dbus-1/services
service_DATA = $(service_in_files:.service.in=.service)
service_in_files = org.gnome.Todo.service.in

EXTRA_DIST=                     \
  appdata/org.gnome.Todo.appdata.xml \
  org.gnome.Todo.Autostart.desktop \
  org.gnome.Todo.desktop \
  todo.gresource.xml \
  gnome-todo.pc.in \
  gtk/menus.ui \
  ui/edit-pane.ui \
  ui/initial-setup.ui \
  ui/list-selector-grid-item.ui \
  ui/list-selector-list-item.ui \
  ui/list-selector-panel.ui \
  ui/list-view.ui \
  ui/new-task-row.ui \
  ui/notification.ui \
  ui/plugin-dialog.ui \
  ui/plugin-row.ui \
  ui/provider-popover.ui \
  ui/provider-row.ui \
  ui/provider-selector.ui \
  ui/task-row.ui \
  ui/window.ui \
  theme/Adwaita.css \
  $(appdata_in_files) \
  $(desktop_in_files) \
  $(gsettings_SCHEMAS) \
  $(service_in_files)

CLEANFILES =                    \
  $(service_DATA)               \
  $(desktop_DATA)               \
  $(autostart_DATA)             \
  $(searchprovider_DATA)        \
  $(appdata_DATA)

-include $(top_srcdir)/git.mk
