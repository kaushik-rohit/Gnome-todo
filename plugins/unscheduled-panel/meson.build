plugin_name = 'unscheduled-panel'

install_dir = join_paths(plugins_install_dir, plugin_name)

install_data(
  join_paths(plugin_name, '__init__.py'),
  install_dir: join_paths(install_dir, plugin_name)
)

configure_file(
  input: '@0@.plugin.in'.format(plugin_name),
  output: '@0@.plugin'.format(plugin_name),
  install: true,
  install_dir: install_dir,
  configuration: plugins_conf
)
