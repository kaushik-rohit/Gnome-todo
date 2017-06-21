plugin_name = 'eds'

plugins_ldflags += ['-Wl,--undefined=gtd_plugin_eds_register_types']

sources = files(
  'gtd-plugin-' + plugin_name + '.c',
  'gtd-provider-' + plugin_name + '.c',
  'gtd-provider-goa.c',
  'gtd-provider-local.c',
  'gtd-task-list-' + plugin_name + '.c'
)

plugins_libs += static_library(
  plugin_name,
  sources: sources,
  include_directories: plugins_incs,
  dependencies: gnome_todo_deps
)

plugin_data = plugin_name + '.plugin'

plugins_confs += configure_file(
  input: plugin_data + '.in',
  output: plugin_data,
  configuration: plugins_conf
)
