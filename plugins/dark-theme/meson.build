plugin_name = 'dark-theme'

sources = [
  'gtd-plugin-dark-theme.c',
  'gtd-plugin-dark-theme.h'
]

libdarktheme_lib = static_library(
  'darktheme',
  sources: sources,
  include_directories: plugins_inc,
  dependencies: gnome_todo_dep,
)

plugins_files += configure_file(
  input: '@0@.plugin.in'.format(plugin_name),
  output: '@0@.plugin'.format(plugin_name),
  configuration: plugins_conf
)
