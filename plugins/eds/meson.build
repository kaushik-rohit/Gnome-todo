plugin_name = 'eds'

sources = [
  'gtd-plugin-eds.c',
  'gtd-plugin-eds.h',
  'gtd-provider-eds.c',
  'gtd-provider-eds.h',
  'gtd-provider-goa.c',
  'gtd-provider-goa.h',
  'gtd-provider-local.c',
  'gtd-provider-local.h',
  'gtd-task-list-eds.c',
  'gtd-task-list-eds.h'
]

libeds_lib = static_library(
  plugin_name,
  sources: sources,
  include_directories: plugins_inc,
  dependencies: gnome_todo_dep,
)

plugins_files += configure_file(
  input: '@0@.plugin.in'.format(plugin_name),
  output: '@0@.plugin'.format(plugin_name),
  configuration: plugins_conf
)
