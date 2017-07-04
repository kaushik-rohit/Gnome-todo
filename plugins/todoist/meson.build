plugin_name = 'todoist'

plugins_ldflags += ['-Wl,--undefined=gtd_plugin_todoist_register_types']

sources = files(
  'gtd-plugin-' + plugin_name + '.c',
  'gtd-provider-' + plugin_name + '.c',
  'gtd-' + plugin_name + '-preferences-panel.c'
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
