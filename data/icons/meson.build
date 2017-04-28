icon_sizes = [
  '16x16',
  '22x22',
  '32x32',
  '48x48',
  '256x256',
  '512x512'
]

foreach icon_size: icon_sizes
  install_data(
    join_paths('hicolor', icon_size, 'apps', 'org.gnome.Todo.png'),
    install_dir: join_paths(gnome_todo_datadir, 'icons', 'hicolor', icon_size, 'apps')
  )
endforeach

install_data(
  join_paths('hicolor', 'symbolic', 'apps', 'org.gnome.Todo-symbolic.svg'),
  install_dir: join_paths(gnome_todo_datadir, 'icons', 'hicolor', 'symbolic', 'apps')
)

meson.add_install_script('meson_post_install.py')
