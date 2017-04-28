# gnome.pc.in options
pc_conf = configuration_data()
pc_conf.set('prefix', gnome_todo_prefix)
pc_conf.set('includedir', gnome_todo_includedir)
pc_conf.set('VERSION', gnome_todo_version)

pc = 'gnome-todo.pc'

configure_file(
  input: '@0@.in'.format(pc),
  output: pc,
  install: true,
  install_dir: join_paths(gnome_todo_libdir, 'pkgconfig'),
  configuration: pc_conf
)

msgfmt = find_program('msgfmt')

po_dir = join_paths(meson.source_root(), 'po')

desktop = 'org.gnome.Todo.desktop'

custom_target(
  desktop,
  input: '@0@.in'.format(desktop),
  output: desktop,
  command: [msgfmt, '--desktop', '--template', '@INPUT@', '-d', po_dir, '-o', '@OUTPUT@'],
  install: true,
  install_dir: join_paths(gnome_todo_datadir, 'applications')
)

autostart = 'org.gnome.Todo.Autostart.desktop'

custom_target(
  autostart,
  input: '@0@.in'.format(autostart),
  output: autostart,
  command: [msgfmt, '--desktop', '--template', '@INPUT@', '-d', po_dir, '-o', '@OUTPUT@'],
  install: true,
  install_dir: gnome_todo_pkgdatadir
)

service_conf = configuration_data()
service_conf.set('bindir', gnome_todo_bindir)

service = 'org.gnome.Todo.service'

configure_file(
  input: '@0@.in'.format(service),
  output: service,
  install: true,
  install_dir: join_paths(gnome_todo_datadir, 'dbus-1', 'services'),
  configuration: service_conf
)

enums = 'org.gnome.todo.enums.xml'

custom_target(
  enums,
  input: join_paths(meson.source_root(), 'src', 'gtd-enums.h'),
  output: enums,
  command: [find_program('glib-mkenums'),
            '--comments', '<!-- @comment@ -->',
            '--fhead', '<schemalist>',
            '--vhead', '  <@type@ id="org.gnome.todo.@EnumName@">',
            '--vprod', '    <value nick="@valuenick@" value="@valuenum@"/>',
            '--vtail', '  </@type@>',
            '--ftail', '</schemalist>',
            '--output', '@OUTPUT@',
            '@INPUT@'],
  install: true,
  install_dir: gnome_todo_schemadir
)

install_data(
  'org.gnome.todo.gschema.xml',
  install_dir: gnome_todo_schemadir
)
meson.add_install_script('meson_post_install.py')

subdir('appdata')
subdir('icons')
