project(
  'gnome-todo', 'c',
  version: '3.25.1',
  license: 'GPL3+',
  default_options: [
    'buildtype=debugoptimized',
    'c_std=gnu99',
    'warning_level=1'
  ],
  meson_version: '>= 0.38.1'
)

gnome_todo_version = meson.project_version()
version_array = gnome_todo_version.split('.')
gnome_todo_major_version = version_array[0].to_int()
gnome_todo_minor_version = version_array[1].to_int()
gnome_todo_micro_version = version_array[2].to_int()

gnome_todo_gir_ns = 'Gtd'
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

gnome_todo_schemadir = join_paths(gnome_todo_datadir, 'glib-2.0', 'schemas')

cc = meson.get_compiler('c')

conf = configuration_data()

conf.set('ENABLE_NLS', get_option('enable-nls'))
conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())

# debug options
conf.set('GNOME_TODO_ENABLE_DEBUG', get_option('buildtype') != 'release')
conf.set('NDEBUG', get_option('buildtype') == 'release')

# package
conf.set_quoted('PACKAGE', meson.project_name())
conf.set_quoted('PACKAGE_BUGREPORT', 'https://bugzilla.gnome.org/enter_bug.cgi?product=gnome-todo')
conf.set_quoted('PACKAGE_NAME', meson.project_name())
conf.set_quoted('PACKAGE_STRING', '@0@-@1@'.format(meson.project_name(), gnome_todo_version))
conf.set_quoted('PACKAGE_TARNAME', gnome_todo_version)
conf.set_quoted('PACKAGE_URL', '')
conf.set_quoted('PACKAGE_VERSION', gnome_todo_version)
conf.set_quoted('PACKAGE_API_VERSION', gnome_todo_version)
conf.set_quoted('VERSION', gnome_todo_version)

# headers
conf.set('HAVE_DLFCN_H', cc.has_header('dlfcn.h'))
conf.set('HAVE_INTTYPES_H', cc.has_header('inttypes.h'))
conf.set('HAVE_MATH_H', cc.has_header('math.h'))
conf.set('HAVE_MEMORY_H', cc.has_header('memory.h'))
conf.set('HAVE_STDINT_H', cc.has_header('stdint.h'))
conf.set('HAVE_STDLIB_H', cc.has_header('stdlib.h'))
conf.set('HAVE_STRINGS_H', cc.has_header('strings.h'))
conf.set('HAVE_STRING_H', cc.has_header('string.h'))
conf.set('HAVE_SYS_STAT_H', cc.has_header('sys/stat.h'))
conf.set('HAVE_SYS_TYPES_H', cc.has_header('sys/types.h'))
conf.set('HAVE_UNISTD_H', cc.has_header('unistd.h'))

# functions
conf.set('HAVE_DCGETTEXT', cc.has_function('dcgettext'))
conf.set('HAVE_GETTEXT', cc.has_function('gettext'))
conf.set('HAVE_ICONV', cc.has_function('iconv'))

# Compiler flags
test_cflags = [
  '-fno-strict-aliasing',
  '-Wall',
  '-Warray-bounds',
  '-Wcast-align',
  '-Wdeclaration-after-statement',
  '-Wextra',
  '-Wformat-nonliteral',
  '-Wformat-security',
  '-Wformat=2',
  '-Wimplicit-function-declaration',
  '-Winit-self',
  '-Winline',
  '-Wmissing-declarations',
  '-Wmissing-format-attribute',
  '-Wmissing-include-dirs',
  '-Wmissing-noreturn',
  '-Wmissing-prototypes',
  '-Wnested-externs',
  '-Wno-error=missing-field-initializers',
  '-Wno-error=unused-parameter',
  '-Wno-missing-field-initializers',
  '-Wno-unused-parameter',
  '-Wold-style-definition',
  '-Wpacked',
  '-Wpointer-arith',
  '-Wredundant-decls',
  '-Wreturn-type',
  '-Wshadow',
  '-Wsign-compare',
  '-Wstrict-aliasing',
  '-Wstrict-prototypes',
  '-Wswitch-default',
  '-Wswitch-enum',
  '-Wundef',
  '-Wunused-but-set-variable',
  '-Wwrite-strings'
]

common_flags = [
  '-DHAVE_CONFIG_H'
]

foreach cflag: test_cflags
  if cc.has_argument(cflag)
    common_flags += [cflag]
  endif
endforeach

add_global_arguments(common_flags, language: 'c')

glib_dep =  dependency('glib-2.0', version: '>= 2.43.4')
gtk_dep = dependency('gtk+-3.0', version: '>= 3.22.0')

gnome_todo_dep = [
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
  output : 'config.h',
  configuration : conf
)

gnome = import('gnome')
pkg = import('pkgconfig')

top_inc = include_directories('.')
src_inc = include_directories('src')

subdir('plugins')
subdir('src')
subdir('data')

if get_option('enable-gtk-doc')
  subdir('doc')
endif

if get_option('enable-nls')
  i18n = import('i18n')

  subdir('po')
endif
