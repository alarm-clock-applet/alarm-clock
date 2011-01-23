# ***************************************************************************
# *** configure.ac for ALARM-CLOCK                                        ***
# ***************************************************************************

m4_define([alarm_clock_major_version], [0])
m4_define([alarm_clock_minor_version], [3])
m4_define([alarm_clock_micro_version], [1])

m4_define([alarm_clock_version],
          [alarm_clock_major_version.alarm_clock_minor_version.alarm_clock_micro_version])
          
AC_INIT([alarm-clock], [alarm_clock_version],
		[https://bugs.launchpad.net/alarm-clock],
		[alarm-clock-applet])
		
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([foreign 1.11 -Wall silent-rules])
AM_MAINTAINER_MODE([enable])
AM_SILENT_RULES([yes])


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

# *****
# GNOME
# *****

GNOME_COMMON_INIT
GNOME_DEBUG_CHECK
GNOME_COMPILE_WARNINGS([maximum])
GNOME_MAINTAINER_MODE_DEFINES


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
   libnotify >= 0.4.1
   libxml-2.0
   unique-1.0])

# Check for gconftool
AC_PATH_PROG(GCONFTOOL, gconftool-2, no)

if test x"$GCONFTOOL" = xno; then
  AC_MSG_ERROR([gconftool-2 executable not found in your path - is gconf installed?])
fi

AM_GCONF_SOURCE_2

#
# Check for libnotify 0.7
#
PKG_CHECK_MODULES(LIBNOTIFY_0_7, libnotify >= 0.7, have_libnotify_0_7=yes, have_libnotify_0_7=no)

if test x$have_libnotify_0_7 = xyes ; then
    AC_DEFINE(HAVE_LIBNOTIFY_0_7, 1, [Have libnotify 0.7])
fi
AM_CONDITIONAL(HAVE_LIBNOTIFY_0_7, test x$have_libnotify_0_7 = xyes)

#
# Check for AppIndicator (optional)
#
APPINDICATOR_REQUIRED=0.0.7

AC_ARG_ENABLE(appindicator,
              AS_HELP_STRING([--enable-appindicator[=@<:@no/auto/yes@:>@]],[Build support for application indicators ]),
              [enable_appindicator=$enableval],
              [enable_appindicator="auto"])

if test x$enable_appindicator = xauto ; then
    PKG_CHECK_EXISTS([appindicator-0.1 >= $APPINDICATOR_REQUIRED],
                     enable_appindicator="yes",
                     enable_appindicator="no")
fi

if test x"$enable_appindicator" = xyes ; then
    PKG_CHECK_EXISTS([appindicator-0.1],,
                     AC_MSG_ERROR([appindicator-0.1 is not installed]))
    PKG_CHECK_MODULES(APP_INDICATOR,
                    appindicator-0.1 >= $APPINDICATOR_REQUIRED)
#    AC_SUBST(APP_INDICATOR_CFLAGS)
#    AC_SUBST(APP_INDICATOR_LIBS)
    AC_DEFINE(HAVE_APP_INDICATOR, 1, [Have AppIndicator])
fi
AM_CONDITIONAL(HAVE_APP_INDICATOR, test x"$enable_appindicator" = xyes)

# ********************
# Internationalisation
# ********************

IT_PROG_INTLTOOL([0.40.0])

GETTEXT_PACKAGE=alarm-clock-applet
AC_SUBST([GETTEXT_PACKAGE])
#ALL_LINGUAS=""
AM_GLIB_GNU_GETTEXT
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [gettext domain])
#localedir='$(datadir)/locale'

# *******************
# Run in source tree?
# *******************
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
				 data/icons/Makefile
				 po/Makefile.in])
AC_OUTPUT

# ****************************

echo "
*************************************************

$PACKAGE_NAME $VERSION 

Configure summary:
    prefix:                 ${prefix}
    source code location:   ${srcdir}
    application indicators: ${enable_appindicator}

*************************************************
"
