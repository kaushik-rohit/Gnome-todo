plugin_name = 'dark-theme'

plugins_ldflags += ['-Wl,--undefined=gtd_plugin_dark_theme_register_types']

plugins_libs += static_library(
  'darktheme',
  sources: 'gtd-plugin-' + plugin_name + '.c',
  include_directories: plugins_incs,
  dependencies: gnome_todo_deps
)

plugin_data = plugin_name + '.plugin'

plugins_confs += configure_file(
  input: plugin_data + '.in',
  output: plugin_data,
  configuration: plugins_conf
)
