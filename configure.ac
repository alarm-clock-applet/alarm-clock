# ***************************************************************************
# *** configure.ac for ALARM-CLOCK                                        ***
# ***************************************************************************

m4_define([alarm_clock_major_version], [0])
m4_define([alarm_clock_minor_version], [2])
m4_define([alarm_clock_micro_version], [6])

m4_define([alarm_clock_version],
          [alarm_clock_major_version.alarm_clock_minor_version.alarm_clock_micro_version])
          
AC_INIT([alarm-clock], [alarm_clock_version],
		[Johannes H. Jensen <joh@pseudoberries.com>],
		[alarm-clock-applet])
		
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.9 foreign])
AM_MAINTAINER_MODE([enable])


# Substitute variables
ALARM_CLOCK_MAJOR_VERSION=alarm_clock_major_version
ALARM_CLOCK_MINOR_VERSION=alarm_clock_minor_version
ALARM_CLOCK_MICRO_VERSION=alarm_clock_micro_version
AC_SUBST(ALARM_CLOCK_MAJOR_VERSION)
AC_SUBST(ALARM_CLOCK_MINOR_VERSION)
AC_SUBST(ALARM_CLOCK_MICRO_VERSION)


PKG_PROG_PKG_CONFIG
AC_PROG_CC
AC_PROG_INSTALL
#IT_PROG_INTLTOOL([0.40.0])

# *****
# GNOME
# *****

GNOME_COMMON_INIT
GNOME_DEBUG_CHECK
GNOME_COMPILE_WARNINGS([maximum])
GNOME_MAINTAINER_MODE_DEFINES

#AM_GLIB_GNU_GETTEXT


# ***************************
# Check for required packages
# ***************************

GLIB_REQUIRED=2.16.0
GTK_REQUIRED=2.12.0

PKG_CHECK_MODULES(BASE, [glib-2.0 >= $GLIB_REQUIRED gmodule-2.0])
PKG_CHECK_MODULES(GTK, [gtk+-2.0 >= $GTK_REQUIRED])
PKG_CHECK_MODULES(GSTREAMER, [gstreamer-0.10])
PKG_CHECK_MODULES(GNOME, 
  [gconf-2.0
   gio-2.0
   gnome-icon-theme
   libgnome-2.0
   libgnomeui-2.0])

# LIBXML_REQUIRED=2.5.0

# Check for libnotify (optional)
AC_ARG_WITH(libnotify,
  AC_HELP_STRING([--without-libnotify],
                 [disable the use of libnotify]),,
  with_libnotify=auto)

have_libnotify="no"
if test "x$with_libnotify" != "xno"; then
  PKG_CHECK_MODULES(LIBNOTIFY, [libnotify],
    have_libnotify="yes", have_libnotify="no")
  
  if test "x$have_libnotify" = "xyes"; then
	AC_DEFINE(HAVE_LIBNOTIFY, 1, [libnotify available])
  fi
fi

#AC_SUBST(LIBNOTIFY_CFLAGS)
#AC_SUBST(LIBNOTIFY_LIBS)

# Check for gconftool
AC_PATH_PROG(GCONFTOOL, gconftool-2, no)

if test x"$GCONFTOOL" = xno; then
  AC_MSG_ERROR([gconftool-2 executable not found in your path - is gconf installed?])
fi

AM_GCONF_SOURCE_2

# Run in source tree?
AC_ARG_ENABLE(run-in-source-tree,
  AC_HELP_STRING([--disable-run-in-source-tree],
                 [disable search for UI files and plugins in local directory]),,
  enable_run_in_source_tree=yes)
  
if test x$enable_run_in_source_tree = xyes; then
  AC_DEFINE(ALARM_CLOCK_RUN_IN_SOURCE_TREE, 1, [enable search for UI files and plugins in local directory])
fi

# ****************************

AC_CONFIG_FILES([Makefile
				 src/Makefile
				 src/tests/Makefile
				 data/Makefile
				 data/icons/Makefile])
AC_OUTPUT

# ****************************

echo "
*************************************************

$PACKAGE_NAME $VERSION 

Configure summary:
    prefix:                 ${prefix}
    source code location:   ${srcdir}
	
    Notifications (libnotify): $have_libnotify

*************************************************
"
