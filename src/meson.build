engine_headers = [
  join_paths('engine', 'gtd-manager.h')
]

install_headers(
  engine_headers,
  subdir: join_paths(gnome_todo_pkgincludedir, 'engine')
)

interfaces_headers = [
  join_paths('interfaces', 'gtd-activatable.h'),
  join_paths('interfaces', 'gtd-panel.h'),
  join_paths('interfaces', 'gtd-provider.h')
]

install_headers(
  interfaces_headers,
  subdir: join_paths(gnome_todo_pkgincludedir, 'interfaces')
)

notification_headers = [
  join_paths('notification', 'gtd-notification.h')
]

install_headers(
  notification_headers,
  subdir: join_paths(gnome_todo_pkgincludedir, 'notification')
)

headers = [
  'gtd-enums.h',
  'gtd-object.h',
  'gtd-task.h',
  'gtd-task-list.h',
  'gtd-task-list-view.h',
  'gtd-timer.h',
  'gtd-types.h',
  'gtd-window.h',
  'gnome-todo.h'
]

install_headers(
  headers,
  subdir: gnome_todo_pkgincludedir
)

private_headers = [
  join_paths('engine', 'gtd-manager-protected.h'),
  join_paths('engine', 'gtd-plugin-manager.h'),
  join_paths('notification', 'gtd-notification-widget.h'),
  join_paths('provider', 'gtd-provider-popover.h'),
  join_paths('provider', 'gtd-provider-row.h'),
  join_paths('provider', 'gtd-provider-selector.h'),
  join_paths('views', 'gtd-list-selector.h'),
  join_paths('views', 'gtd-list-selector-grid.h'),
  join_paths('views', 'gtd-list-selector-grid-item.h'),
  join_paths('views', 'gtd-list-selector-item.h'),
  join_paths('views', 'gtd-list-selector-list.h'),
  join_paths('views', 'gtd-list-selector-list-item.h'),
  join_paths('views', 'gtd-list-selector-panel.h'),
  'gtd-application.h',
  'gtd-arrow-frame.h',
  'gtd-dnd-row.h',
  'gtd-edit-pane.h',
  'gtd-empty-list-widget.h',
  'gtd-initial-setup-window.h',
  'gtd-new-task-row.h',
  'gtd-plugin-dialog.h',
  'gtd-plugin-dialog-row.h',
  'gtd-task-row.h',
  'main.c'
]

sources = [
  join_paths('engine', 'gtd-manager.c'),
  join_paths('engine', 'gtd-plugin-manager.c'),
  join_paths('interfaces', 'gtd-activatable.c'),
  join_paths('interfaces', 'gtd-panel.c'),
  join_paths('interfaces', 'gtd-provider.c'),
  join_paths('notification', 'gtd-notification.c'),
  join_paths('notification', 'gtd-notification-widget.c'),
  join_paths('provider', 'gtd-provider-popover.c'),
  join_paths('provider', 'gtd-provider-row.c'),
  join_paths('provider', 'gtd-provider-selector.c'),
  join_paths('views', 'gtd-list-selector.c'),
  join_paths('views', 'gtd-list-selector-grid.c'),
  join_paths('views', 'gtd-list-selector-grid-item.c'),
  join_paths('views', 'gtd-list-selector-item.c'),
  join_paths('views', 'gtd-list-selector-list.c'),
  join_paths('views', 'gtd-list-selector-list-item.c'),
  join_paths('views', 'gtd-list-selector-panel.c'),
  'gtd-application.c',
  'gtd-arrow-frame.c',
  'gtd-dnd-row.c',
  'gtd-edit-pane.c',
  'gtd-empty-list-widget.c',
  'gtd-initial-setup-window.c',
  'gtd-new-task-row.c',
  'gtd-object.c',
  'gtd-plugin-dialog.c',
  'gtd-plugin-dialog-row.c',
  'gtd-task.c',
  'gtd-task-list.c',
  'gtd-task-list-view.c',
  'gtd-task-row.c',
  'gtd-timer.c',
  'gtd-window.c',
  'main.c'
]

data_dir = join_paths(meson.source_root(), 'data')

sources += gnome.compile_resources(
  'gtd-resources',
  join_paths(data_dir, 'todo.gresource.xml'),
  dependencies: plugins_files,
  export: true,
  source_dir: [
    data_dir,
    join_paths(meson.build_root(), 'plugins')
  ],
  c_name: 'todo',
)

enum = 'gtd-enum-types'

sources += gnome.mkenums(
  enum,
  sources: 'gtd-enums.h',
  c_template: '@0@.c.template'.format(enum),
  h_template: '@0@.h.template'.format(enum),
)

ldflags = [
  '-Wl,--export-dynamic',
  '-Wl,--undefined=gtd_plugin_eds_register_types'
]

gnome_todo_libs = [
  libeds_lib
]

if get_option('enable-background-plugin')
  ldflags += ['-Wl,--undefined=gtd_plugin_background_register_types']
  gnome_todo_libs += [libbackground_lib]
endif

if get_option('enable-dark-theme-plugin')
  ldflags += ['-Wl,--undefined=gtd_plugin_dark_theme_register_types']
  gnome_todo_libs += [libdarktheme_lib]
endif

if get_option('enable-scheduled-panel-plugin')
  ldflags += ['-Wl,--undefined=gtd_plugin_scheduled_panel_register_types']
  gnome_todo_libs += [libscheduledpanel_lib]
endif

if get_option('enable-today-panel-plugin')
  ldflags += ['-Wl,--undefined=gtd_plugin_today_panel_register_types']
  gnome_todo_libs += [libtodaypanel_lib]
endif

if get_option('enable-todo-txt-plugin')
  ldflags += ['-Wl,--undefined=gtd_plugin_todo_txt_register_types']
  gnome_todo_libs += [libtodotxt_lib]
endif

gnome_todo_ldflags = []

if host_machine.system().contains('linux')
  foreach ldflag: ldflags
    if cc.has_argument(ldflag)
      gnome_todo_ldflags += ldflag
    endif
  endforeach
endif

gnome_todo_cflags = common_flags + [
  '-DPACKAGE_LOCALE_DIR="@0@"'.format(gnome_todo_localedir),
  '-DPACKAGE_DATA_DIR="@0@"'.format(gnome_todo_pkgdatadir),
  '-DPACKAGE_LIB_DIR="@0@"'.format(gnome_todo_pkglibdir),
  '-DUI_DATA_DIR="@0@"'.format(join_paths(gnome_todo_pkgdatadir, 'style'))
]

gnome_todo_inc = [
  top_inc,
  include_directories(
    'engine',
    'provider',
    'notification',
    'interfaces',
    'views'
  )
]

gnome_todo = executable(
  'gnome-todo',
  sources + headers + private_headers,
  include_directories: gnome_todo_inc,
  dependencies: gnome_todo_dep,
  c_args: gnome_todo_cflags,
  link_with: gnome_todo_libs,
  link_args: gnome_todo_ldflags,
  install: true,
  install_dir: gnome_todo_bindir
)

gtd_ldflags = [
  '-Wl,--export-dynamic'
]

libgtd_lib = shared_library(
  'gtd',
  sources: sources + headers + private_headers,
  include_directories: gnome_todo_inc,
  dependencies: gnome_todo_dep,
  c_args: gnome_todo_cflags,
  link_args: gtd_ldflags
)

libgtd_inc = include_directories('.')
libgtd_dep = declare_dependency(
  link_with: libgtd_lib,
  include_directories: libgtd_inc,
  dependencies: gnome_todo_dep
)

if get_option('enable-introspection')
  gir_sources = [
    join_paths('engine', 'gtd-manager.c'),
    join_paths('engine', 'gtd-manager.h'),
    join_paths('interfaces', 'gtd-activatable.c'),
    join_paths('interfaces', 'gtd-activatable.h'),
    join_paths('interfaces', 'gtd-panel.c'),
    join_paths('interfaces', 'gtd-panel.h'),
    join_paths('interfaces', 'gtd-provider.c'),
    join_paths('interfaces', 'gtd-provider.h'),
    join_paths('notification', 'gtd-notification.c'),
    join_paths('notification', 'gtd-notification.h'),
    'gtd-enums.h',
    'gtd-object.c',
    'gtd-object.h',
    'gtd-task.c',
    'gtd-task.h',
    'gtd-task-list.c',
    'gtd-task-list.h',
    'gtd-task-list-view.c',
    'gtd-task-list-view.h',
    'gtd-timer.c',
    'gtd-timer.h',
    'gtd-window.c',
    'gtd-window.h',
    'gtd-types.h'
  ]

  gir_inc = [
    'Gio-2.0',
    'GObject-2.0',
    'Gtk-3.0'
  ]

  gir_extra_args = [
    '--warn-all'
  ]

  gir_dir = join_paths(gnome_todo_datadir, '@0@-@1@'.format('gir', gnome_todo_gir_version))
  typelib_dir = join_paths(gnome_todo_libdir, '@0@-@1@'.format('girepository', gnome_todo_gir_version))

  gnome.generate_gir(
    gnome_todo,
    sources: gir_sources,
    namespace: gnome_todo_gir_ns,
    nsversion: gnome_todo_gir_version,
    identifier_prefix: gnome_todo_gir_ns,
    symbol_prefix: gnome_todo_gir_ns.to_lower(),
    includes: gir_inc,
    include_directories: gnome_todo_inc,
    install: true,
    install_dir_gir: gir_dir,
    install_dir_typelib: typelib_dir,
    extra_args: gir_extra_args
  )
endif
