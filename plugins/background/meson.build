plugin_name = 'background'

sources = [
  'gtd-plugin-background.c',
  'gtd-plugin-background.h'
]

libbackground_lib = static_library(
  plugin_name,
  sources: sources,
  include_directories: plugins_inc,
  dependencies: gnome_todo_dep,
  c_args: common_flags + [
    '-DPACKAGE_DATA_DIR="@0@"'.format(gnome_todo_pkgdatadir)
  ]
)

install_data(
  'org.gnome.todo.background.gschema.xml',
  install_dir: gnome_todo_schemadir
)

plugins_files += configure_file(
  input: '@0@.plugin.in'.format(plugin_name),
  output: '@0@.plugin'.format(plugin_name),
  configuration: plugins_conf
)
