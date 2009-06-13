include $(top_srcdir)/Makefile.decl

soundsdir= $(datadir)/sounds
gladedir = $(pkgdatadir)/glade

SUBDIRS = icons ui tests


#
# alarm-clock-applet binary
#

libexec_PROGRAMS = alarm-clock-applet

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

alarm_clock_applet_CFLAGS =				\
	-DALARM_DATADIR="\"$(datadir)\""	\
	-DALARM_GLADEDIR="\"$(gladedir)\""	\
	-DALARM_SOUNDSDIR="\"$(soundsdir)\""\
	$(ALARM_CLOCK_CFLAGS)				\
	$(LIBNOTIFY_CFLAGS)					\
	$(WARN_CFLAGS)						\
	$(AM_CFLAGS)

alarm_clock_applet_LDADD =		\
	$(ALARM_CLOCK_LIBS)			\
	$(LIBNOTIFY_LIBS)


#
# Bonobo server file
#
serverdir       = $(libdir)/bonobo/servers
server_in_files = AlarmClock.server.in
server_DATA     = $(server_in_files:.server.in=.server)

$(server_DATA): $(server_in_files)
	sed -e "s|\@LIBEXECDIR\@|$(libexecdir)|" $< > $@




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
