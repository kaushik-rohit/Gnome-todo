plugin_name = 'todoist'

sources = [
  'gtd-plugin-todoist.c',
  'gtd-plugin-todoist.h',
  'gtd-provider-todoist.c',
  'gtd-provider-todoist.h',
  'gtd-todoist-preferences-panel.c',
  'gtd-todoist-preferences-panel.h'
]

libtodoist_lib = static_library(
  'todoist',
  sources: sources,
  include_directories: plugins_inc,
  dependencies: gnome_todo_dep,
  c_args: common_flags + [
    '-DPACKAGE_DATA_DIR="@0@"'.format(gnome_todo_pkgdatadir)
  ]
)

plugins_files += configure_file(
  input: '@0@.plugin.in'.format(plugin_name),
  output: '@0@.plugin'.format(plugin_name),
  configuration: plugins_conf
)
