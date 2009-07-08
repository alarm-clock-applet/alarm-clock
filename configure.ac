# ***************************************************************************
# *** configure.ac for ALARM-CLOCK                                        ***
# ***************************************************************************

AC_INIT([Alarm Clock], [0.2.6],
		[Johannes H. Jensen <joh@pseudoberries.com>],
		[alarm-clock-applet])
		
#AM_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR(m4)

AM_INIT_AUTOMAKE([1.9 foreign])
AM_MAINTAINER_MODE

PKG_PROG_PKG_CONFIG
AC_PROG_CC
AC_PROG_INSTALL
#IT_PROG_INTLTOOL([0.40.0])

GNOME_COMMON_INIT
GNOME_DEBUG_CHECK
GNOME_COMPILE_WARNINGS([maximum])
GNOME_MAINTAINER_MODE_DEFINES

#AM_GLIB_GNU_GETTEXT

GLIB_REQUIRED=2.13.0
GTK_REQUIRED=2.11.0
GNOME_VFS_REQUIRED=2.15.4
GCONF_REQUIRED=2.8.0
LIBGLADE_REQUIRED=2.4.0
GST_REQUIRED=0.10.2
LIBGNOME_REQUIRED=2.8.0
LIBGNOMEUI_REQUIRED=2.8.0
LIBPANEL_REQUIRED=2.13.4
GNOME_ICON_THEME_REQUIRED=2.15.91
LIBNOTIFY_REQUIRED=0.3.2

# LIBXML_REQUIRED=2.5.0

PKG_CHECK_MODULES([ALARM_CLOCK],
  [glib-2.0 >= $GLIB_REQUIRED
   gtk+-2.0 >= $GTK_REQUIRED
   gnome-vfs-2.0 >= $GNOME_VFS_REQUIRED
   gconf-2.0 >= $GCONF_REQUIRED
   libglade-2.0 >= $LIBGLADE_REQUIRED
   gstreamer-0.10 >= $GST_REQUIRED
   libpanelapplet-2.0 >= $LIBPANEL_REQUIRED
   gnome-icon-theme >= $GNOME_ICON_THEME_REQUIRED
   libgnome-2.0 >= $LIBGNOME_REQUIRED
   libgnomeui-2.0 >= $LIBGNOMEUI_REQUIRED])

# Check for libnotify (optional)
LIBNOTIFY_CFLAGS=
LIBNOTIFY_LIBS=
PKG_CHECK_MODULES(LIBNOTIFY, libnotify >= $LIBNOTIFY_REQUIRED,
		  HAVE_LIBNOTIFY="yes", HAVE_LIBNOTIFY="no")

if test "x$HAVE_LIBNOTIFY" = "xyes"; then
	AC_DEFINE(HAVE_LIBNOTIFY, 1, [libnotify available])
fi

AC_SUBST(LIBNOTIFY_CFLAGS)
AC_SUBST(LIBNOTIFY_LIBS)

# GConf
AC_PATH_PROG(GCONFTOOL, gconftool-2, no)

if test x"$GCONFTOOL" = xno; then
  AC_MSG_ERROR([gconftool-2 executable not found in your path - should be installed with GConf])
fi

AM_GCONF_SOURCE_2


# ****************************

AC_CONFIG_FILES([Makefile
				 src/Makefile
				 src/ui/Makefile
				 src/icons/Makefile
				 src/tests/Makefile])
AC_OUTPUT

# ****************************

echo "
*************************************************

$PACKAGE_NAME $VERSION 

Configure summary:
    prefix:                 ${prefix}
    source code location:   ${srcdir}
	
    Notifications (libnotify): $HAVE_LIBNOTIFY

*************************************************
"
