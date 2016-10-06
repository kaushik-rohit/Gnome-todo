dnl -*- mode: m4 -*-
AC_PREREQ([2.69])
AC_INIT([gnome-todo],[3.19.92],[https://bugzilla.gnome.org/enter_bug.cgi?product=gnome-todo],[gnome-todo])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([1.11.1 foreign -Wno-portability dist-xz no-dist-gzip tar-ustar subdir-objects])
AM_MAINTAINER_MODE([enable])
AM_SILENT_RULES([yes])

# Releases do not have git directories
AX_IS_RELEASE([git-directory])

## don't rerun to this point if we abort
AC_CACHE_SAVE

# We require additional macros later on
AX_REQUIRE_DEFINED([APPSTREAM_XML])
AX_REQUIRE_DEFINED([IT_PROG_INTLTOOL])

# Add debug symbol
AX_CHECK_ENABLE_DEBUG([yes], [GNOME_TODO_ENABLE_DEBUG])

# Enable compiler warnings
AX_COMPILER_FLAGS([GNOME_TODO_WARN_CFLAGS], [GNOME_TODO_WARN_LDFLAGS])

AC_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_SED

# enable libtool
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

## don't rerun to this point if we abort
AC_CACHE_SAVE

PKG_PROG_PKG_CONFIG

GLIB_GSETTINGS

dnl ***************************************************************************
dnl Internationalization
dnl ***************************************************************************
IT_PROG_INTLTOOL([0.40.6])

GETTEXT_PACKAGE=AC_PACKAGE_TARNAME
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])

AS_IF([test "x$USE_NLS" != "xno"],
      [AC_DEFINE([ENABLE_NLS], [1], [Native language support is enabled])])

dnl ================================================================
dnl GObject introspection support
dnl ================================================================
GOBJECT_INTROSPECTION_CHECK([1.42.0])


dnl ================================================================
dnl Misc
dnl ================================================================
AC_PATH_PROG(GLIB_MKENUMS, glib-mkenums)

GLIB_COMPILE_RESOURCES=`$PKG_CONFIG --variable glib_compile_resources gio-2.0`
AC_SUBST([GLIB_COMPILE_RESOURCES])

PKG_CHECK_MODULES(GNOME_TODO,
                  gmodule-export-2.0
                  gio-2.0 >= 2.43.4
                  glib-2.0 >= 2.43.4
                  goa-1.0 >= 3.2.0
                  gtk+-3.0 >= 3.22.0
                  libecal-1.2 >= 3.13.90
                  libedataserver-1.2 >= 3.17.1
                  libedataserverui-1.2 >= 3.17.1
                  libical >= 0.43
                  libpeas-1.0 >= 1.17)

APPSTREAM_XML


dnl ================================================================
dnl Plugins
dnl ================================================================
GNOME_TODO_ADD_PLUGIN([eds], [Evolution-Data-Server], [yes])
GNOME_TODO_ADD_PLUGIN([dark-theme], [Dark theme], [yes])
GNOME_TODO_ADD_PLUGIN([scheduled-panel], [Scheduled Panel], [yes])
GNOME_TODO_ADD_PLUGIN([score], [Score], [yes])
GNOME_TODO_ADD_PLUGIN([today-panel], [Today Panel], [yes])
GNOME_TODO_ADD_PLUGIN([unscheduled-panel], [Unscheduled Tasks Panel], [yes])

AC_CONFIG_FILES([
      Makefile
      src/Makefile
      data/Makefile
      data/icons/Makefile
      data/gnome-todo.pc
      data/org.gnome.Todo.desktop.in
      plugins/Makefile
      po/Makefile.in
])

AC_OUTPUT

dnl ==========================================================================
echo "

        gnome-todo $VERSION
        ==================

        prefix:        ${prefix}
        compiler:      ${CC}
	global flags:  ${CFLAGS} ${CPPFLAGS} ${LDFLAGS}
        release:       ${ax_is_release}

    Plugins:
        Score ................... ${enable_score_plugin}
        Today ................... ${enable_today_plugin}
        Unscheduled panel ....... ${enable_unscheduled_panel_plugin}

        Now type 'make' to build $PACKAGE
"
