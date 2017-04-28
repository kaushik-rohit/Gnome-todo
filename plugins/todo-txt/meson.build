plugin_name = 'todo-txt'

sources = [
  'gtd-plugin-todo-txt.c',
  'gtd-plugin-todo-txt.h',
  'gtd-provider-todo-txt.c',
  'gtd-provider-todo-txt.h',
  'gtd-todo-txt-parser.c',
  'gtd-todo-txt-parser.h'
]

libtodotxt_lib = static_library(
  'todotxt',
  sources: sources,
  include_directories: plugins_inc,
  dependencies: gnome_todo_dep,
  c_args: common_flags + [
    '-DPACKAGE_DATA_DIR="@0@"'.format(gnome_todo_pkgdatadir)
  ]
)

install_data(
  'org.gnome.todo.txt.gschema.xml',
  install_dir: gnome_todo_schemadir
)

plugins_files += configure_file(
  input: '@0@.plugin.in'.format(plugin_name),
  output: '@0@.plugin'.format(plugin_name),
  configuration: plugins_conf
)
