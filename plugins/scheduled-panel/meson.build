plugin_name = 'scheduled-panel'

sources = [
  'gtd-panel-scheduled.c',
  'gtd-panel-scheduled.h',
  'gtd-plugin-scheduled-panel.c',
  'gtd-plugin-scheduled-panel.h'
]

libscheduledpanel_lib = static_library(
  'scheduledpanel',
  sources: sources,
  include_directories: plugins_inc,
  dependencies: gnome_todo_dep,
)

plugins_files += configure_file(
  input: '@0@.plugin.in'.format(plugin_name),
  output: '@0@.plugin'.format(plugin_name),
  configuration: plugins_conf
)
