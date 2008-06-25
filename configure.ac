dnl ***************************************************************************
dnl *** configure.ac for ALARM-CLOCK                                        ***
dnl ***************************************************************************

AC_INIT([Alarm Clock], [0.2.3],
		[Johannes H. Jensen <joh@pseudoberries.com>],
		[alarm-clock])
		
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE
AC_CONFIG_MACRO_DIR(m4)

AM_MAINTAINER_MODE

dnl ***************************************************************************
dnl *** Minimum library versions for ALARM-CLOCK                            ***
dnl ***************************************************************************

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

dnl LIBXML_REQUIRED=2.5.0

dnl ***************************************************************************

AC_PROG_CC
AC_PROG_INSTALL

dnl IT_PROG_INTLTOOL([0.35.0])
PKG_PROG_PKG_CONFIG([0.19])

GNOME_COMPILE_WARNINGS

dnl ***************************************************************************
dnl *** Use pkg-config to check for dependencies                            ***
dnl ***************************************************************************

dnl -- check for glib/gobject (required) --------------------------------------
PKG_CHECK_MODULES(GLIB, glib-2.0 >= $GLIB_REQUIRED gobject-2.0 >= $GLIB_REQUIRED)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

dnl -- Check for GTK+ 2.0 (required) ------------------------------------------
PKG_CHECK_MODULES(GTK, gtk+-2.0 >= $GTK_REQUIRED)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

dnl -- check for gnome-vfs (required) -----------------------------------------
PKG_CHECK_MODULES(GNOME_VFS, [gnome-vfs-2.0 >= $GNOME_VFS_REQUIRED])
AC_SUBST(GNOME_VFS_CFLAGS)
AC_SUBST(GNOME_VFS_LIBS)

dnl -- check for gconf (required) -----------------------------------------
PKG_CHECK_MODULES(GCONF, [gconf-2.0 >= $GCONF_REQUIRED])
AC_SUBST(GCONF_CFLAGS)
AC_SUBST(GCONF_LIBS)

dnl -- check for libglade (required) ------------------------------------------
PKG_CHECK_MODULES(LIBGLADE, libglade-2.0 >= $LIBGLADE_REQUIRED)
AC_SUBST(LIBGLADE_CFLAGS)
AC_SUBST(LIBGLADE_LIBS)

dnl -- check for gstreamer (required) ------------------------------------------
PKG_CHECK_MODULES(GST, gstreamer-0.10 >= $GST_REQUIRED)
AC_SUBST(GST_CFLAGS)
AC_SUBST(GST_LIBS)

dnl -- check for libpanelapplet (required) ------------------------------------
PKG_CHECK_MODULES(GNOME_APPLETS, libpanelapplet-2.0 >= $LIBPANEL_REQUIRED)
AC_SUBST(GNOME_APPLETS_CFLAGS)
AC_SUBST(GNOME_APPLETS_LIBS)

dnl -- Check for gnome-icon-theme (required) ----------------------------------
PKG_CHECK_MODULES(GIT, gnome-icon-theme >= $GNOME_ICON_THEME_REQUIRED)

dnl -- check for libgnome (required) ------------------------------------------
PKG_CHECK_MODULES(GNOME_LIBS2, libgnome-2.0 >= $LIBGNOME_REQUIRED libgnomeui-2.0 >= $LIBGNOMEUI_REQUIRED)
AC_SUBST(GNOME_LIBS2_CFLAGS)
AC_SUBST(GNOME_LIBS2_LIBS)

dnl -- check for libnotify (optional) -----------------------------------------
LIBNOTIFY_CFLAGS=
LIBNOTIFY_LIBS=
PKG_CHECK_MODULES(LIBNOTIFY, libnotify >= $LIBNOTIFY_REQUIRED,
		  HAVE_LIBNOTIFY="yes", HAVE_LIBNOTIFY="no")

if test "x$HAVE_LIBNOTIFY" = "xyes"; then
	AC_DEFINE(HAVE_LIBNOTIFY, 1, [libnotify available])
fi

AC_SUBST(LIBNOTIFY_CFLAGS)
AC_SUBST(LIBNOTIFY_LIBS)

dnl ***************************************************************************
dnl *** find paths to installed binaries                                    ***
dnl ***************************************************************************

dnl -- gconftool-2 ------------------------------------------------------------
AC_PATH_PROG(GCONFTOOL, gconftool-2, no)
if test x"$GCONFTOOL" = xno; then
  AC_MSG_ERROR([gconftool-2 executable not found in your path - should be installed with GConf])
fi


dnl ***************************************************************************
dnl *** GConf                                                               ***
dnl ***************************************************************************
AM_GCONF_SOURCE_2

dnl ***************************************************************************
dnl *** Honour aclocal flags                                                ***
dnl ***************************************************************************
ACLOCAL_AMFLAGS="\${ACLOCAL_FLAGS}"
AC_SUBST(ACLOCAL_AMFLAGS)

dnl ***************************************************************************
dnl *** Set install directories                                             ***
dnl ***************************************************************************
iconsdir="${datadir}/icons"
AC_SUBST(iconsdir)
pixmapsdir="${datadir}/pixmaps"
AC_SUBST(pixmapsdir)
pkgdatadir="${datadir}/alarm-clock"
AC_SUBST(pkgdatadir)
gladedir="${pkgdatadir}/glade"
AC_SUBST(gladedir)
soundsdir="${datadir}/sounds"
AC_SUBST(soundsdir)

AC_DEFINE_DIR(GNOME_ICONDIR, "${iconsdir}", [Gnome Icon Directory])
AC_DEFINE_DIR(GNOME_PIXMAPSDIR, "${pixmapsdir}", [Gnome Pixmaps Directory])
AC_DEFINE_DIR(GNOME_GLADEDIR, "${gladedir}", [Glade Resources Directory])
AC_DEFINE_DIR(GNOME_SOUNDSDIR, "${soundsdir}", [Gnome Sounds Directory])
AC_DEFINE_DIR(DATADIR, "${datadir}", [datadir])
AC_DEFINE_DIR(SYSCONFDIR, "${sysconfdir}", [sysconfdir])
AC_DEFINE_DIR(LIBDIR, "${libdir}", [libdir])
AC_DEFINE_DIR(PREFIX, "${prefix}", [install prefix])


AC_CONFIG_FILES([Makefile
				 src/Makefile
				 src/tests/Makefile])
AC_OUTPUT

dnl ***************************************************************************
dnl *** Show summary                                                        ***
dnl ***************************************************************************

echo
echo "*************************************************"
echo
echo "Configure summary:"
echo "  Notifications (libnotify): $HAVE_LIBNOTIFY"
echo 
echo "*************************************************"
echo
