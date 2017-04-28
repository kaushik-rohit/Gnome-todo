plugin_name = 'today-panel'

sources = [
  'gtd-panel-today.c',
  'gtd-panel-today.h',
  'gtd-plugin-today-panel.c',
  'gtd-plugin-today-panel.h'
]

libtodaypanel_lib = static_library(
  'todaypanel',
  sources: sources,
  include_directories: plugins_inc,
  dependencies: gnome_todo_dep,
)

plugins_files += configure_file(
  input: '@0@.plugin.in'.format(plugin_name),
  output: '@0@.plugin'.format(plugin_name),
  configuration: plugins_conf
)
