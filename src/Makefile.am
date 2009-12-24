include $(top_srcdir)/Makefile.decl

soundsdir= $(datadir)/sounds
gladedir = $(pkgdatadir)/glade

SUBDIRS = tests


#
# alarm-clock-applet binary
#

bin_PROGRAMS = alarm-clock-applet

alarm_clock_applet_SOURCES = alarm-applet.c alarm-applet.h	\
							 player.c player.h				\
						     util.c util.h					\
						     list-entry.c list-entry.h		\
						     alarm-gconf.c alarm-gconf.h	\
						     ui.c ui.h						\
						     prefs.c prefs.h				\
						     alarms-list.c alarms-list.h	\
						     edit-alarm.c edit-alarm.h		\
						     alarm.c alarm.h

alarm_clock_applet_CFLAGS =	                        \
	-DALARM_CLOCK_DATADIR=\"$(datadir)\"            \
	-DALARM_CLOCK_PKGDATADIR=\"$(pkgdatadir)\"      \
	@BASE_CFLAGS@ @GTK_CFLAGS@ @GSTREAMER_CFLAGS@   \
	@GNOME_CFLAGS@ @LIBNOTIFY_CFLAGS@               \
	@WARN_CFLAGS@ $(AM_CFLAGS)

alarm_clock_applet_LDADD =		\
	@BASE_LIBS@ @GTK_LIBS@ @GSTREAMER_LIBS@         \
	@GNOME_LIBS@ @LIBNOTIFY_LIBS@


#
# GConf schemas
#
schemadir = $(GCONF_SCHEMA_FILE_DIR)
schema_in_files = alarm-clock.schemas.in
schema_DATA = $(schema_in_files:.schemas.in=.schemas)

#@INTLTOOL_SCHEMAS_RULE@
#@INTLTOOL_SERVER_RULE@

#uninstall-data:
#	rm -rf $(pkgdatadir)

if GCONF_SCHEMAS_INSTALL
install-data-local:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_builddir)/src/$(schema_DATA)

uninstall-local:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-uninstall-rule $(top_builddir)/src/$(schema_DATA)
endif

CLEANFILES = $(server_DATA)

EXTRA_DIST += $(server_in_files)	\
			  $(schema_DATA)
