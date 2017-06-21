plugin_name = 'score'

plugin_dir = join_paths(gnome_todo_pluginsdir, plugin_name)

install_data(
  join_paths(plugin_name, '__init__.py'),
  install_dir: join_paths(plugin_dir, plugin_name)
)

plugin_data = plugin_name + '.plugin'

plugins_confs += configure_file(
  input: plugin_data + '.in',
  output: plugin_data,
  install: true,
  install_dir: plugin_dir,
  configuration: plugins_conf
)
