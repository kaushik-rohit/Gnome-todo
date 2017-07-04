project(
  'gnome-todo', 'c',
  version: '3.25.3',
  license: 'GPL3+',
  default_options: [
    'buildtype=debugoptimized',
    'c_std=gnu99',
    'warning_level=1'
  ],
  meson_version: '>= 0.41.0'
)

gnome_todo_version = meson.project_version()
version_array = gnome_todo_version.split('.')
gnome_todo_major_version = version_array[0].to_int()
gnome_todo_minor_version = version_array[1].to_int()
gnome_todo_micro_version = version_array[2].to_int()

gnome_todo_gir_namespace = 'Gtd'
gnome_todo_gir_version = '1.0'

gnome_todo_prefix = get_option('prefix')
gnome_todo_bindir = join_paths(gnome_todo_prefix, get_option('bindir'))
gnome_todo_libdir = join_paths(gnome_todo_prefix, get_option('libdir'))
gnome_todo_localedir = join_paths(gnome_todo_prefix, get_option('localedir'))
gnome_todo_datadir = join_paths(gnome_todo_prefix, get_option('datadir'))
gnome_todo_includedir = join_paths(gnome_todo_prefix, get_option('includedir'))

gnome_todo_pkglibdir = join_paths(gnome_todo_libdir, meson.project_name())
gnome_todo_pkgdatadir = join_paths(gnome_todo_datadir, meson.project_name())
gnome_todo_pkgincludedir = join_paths(gnome_todo_includedir, meson.project_name())

gnome_todo_pluginsdir = join_paths(gnome_todo_pkglibdir, 'plugins')
gnome_todo_schemadir = join_paths(gnome_todo_datadir, 'glib-2.0', 'schemas')

soversion = 0
current = 0
revision = 0
libversion = '@0@.@1@.@2@'.format(soversion, current, revision)

cc = meson.get_compiler('c')

config_h = configuration_data()

config_h.set_quoted('GETTEXT_PACKAGE', meson.project_name())

# debug options
config_h.set('GNOME_TODO_ENABLE_DEBUG', get_option('buildtype') != 'release')
config_h.set('NDEBUG', get_option('buildtype') == 'release')

# package
config_h.set_quoted('PACKAGE', meson.project_name())
config_h.set_quoted('PACKAGE_BUGREPORT', 'https://bugzilla.gnome.org/enter_bug.cgi?product=gnome-todo')
config_h.set_quoted('PACKAGE_NAME', meson.project_name())
config_h.set_quoted('PACKAGE_STRING', '@0@ @1@'.format(meson.project_name(), gnome_todo_version))
config_h.set_quoted('PACKAGE_TARNAME', gnome_todo_version)
config_h.set_quoted('PACKAGE_URL', 'https://wiki.gnome.org/Apps/Todo')
config_h.set_quoted('PACKAGE_VERSION', gnome_todo_version)
config_h.set_quoted('VERSION', gnome_todo_version)

# headers
config_h.set('HAVE_DLFCN_H', cc.has_header('dlfcn.h'))
config_h.set('HAVE_INTTYPES_H', cc.has_header('inttypes.h'))
config_h.set('HAVE_MATH_H', cc.has_header('math.h'))
config_h.set('HAVE_MEMORY_H', cc.has_header('memory.h'))
config_h.set('HAVE_STDINT_H', cc.has_header('stdint.h'))
config_h.set('HAVE_STDLIB_H', cc.has_header('stdlib.h'))
config_h.set('HAVE_STRINGS_H', cc.has_header('strings.h'))
config_h.set('HAVE_STRING_H', cc.has_header('string.h'))
config_h.set('HAVE_SYS_STAT_H', cc.has_header('sys/stat.h'))
config_h.set('HAVE_SYS_TYPES_H', cc.has_header('sys/types.h'))
config_h.set('HAVE_UNISTD_H', cc.has_header('unistd.h'))

# functions
config_h.set('HAVE_DCGETTEXT', cc.has_function('dcgettext'))
config_h.set('HAVE_GETTEXT', cc.has_function('gettext'))
config_h.set('HAVE_ICONV', cc.has_function('iconv'))

# compiler flags
common_flags = ['-DHAVE_CONFIG_H']

buildtype = get_option('buildtype')
if buildtype == 'debug' or buildtype == 'debugoptimized'
  test_cflags = [
    '-fno-strict-aliasing',
    '-Wcast-align',
    '-Wdeclaration-after-statement',
    '-Wformat=2',
    '-Winit-self',
    '-Winline',
    '-Wmissing-declarations',
    '-Wmissing-format-attribute',
    '-Wmissing-include-dirs',
    '-Wmissing-noreturn',
    '-Wmissing-prototypes',
    '-Wnested-externs',
    '-Wno-error=unused-parameter',
    '-Wno-error=missing-field-initializers',
    '-Wno-missing-field-initializers',
    '-Wno-unused-parameter',
    '-Wold-style-definition',
    '-Wpacked',
    '-Wredundant-decls',
    '-Wshadow',
    '-Wstrict-prototypes',
    '-Wswitch-enum',
    '-Wundef',
    '-Wunused-but-set-variable',
    '-Wwrite-strings'
  ]

  foreach cflag: test_cflags
    if cc.has_argument(cflag)
      common_flags += [cflag]
    endif
  endforeach
endif

add_project_arguments(common_flags, language: 'c')

glib_dep = dependency('glib-2.0', version: '>= 2.43.4')
gtk_dep = dependency('gtk+-3.0', version: '>= 3.22.0')

gnome_todo_deps = [
  dependency('gmodule-export-2.0'),
  dependency('gio-2.0', version: '>= 2.43.4'),
  glib_dep,
  dependency('goa-1.0', version: '>= 3.2.0'),
  gtk_dep,
  dependency('libecal-1.2', version: '>= 3.13.90'),
  dependency('libedataserver-1.2', version: '>= 3.17.1'),
  dependency('libedataserverui-1.2', version: '>= 3.17.1'),
  dependency('libical', version: '>= 0.43'),
  dependency('libpeas-1.0', version: '>= 1.17'),
  cc.find_library('m', required: true)
]

configure_file(
  output: 'config.h',
  configuration: config_h
)

gnome = import('gnome')
i18n = import('i18n')
pkg = import('pkgconfig')

top_inc = include_directories('.')
src_inc = include_directories('src')

po_dir = join_paths(meson.source_root(), 'po')

subdir('plugins')
subdir('src')
subdir('data')
subdir('po')

if get_option('enable-gtk-doc')
  subdir('doc/reference')
endif

meson.add_install_script('meson_post_install.py')
