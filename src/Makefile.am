INCLUDES =						\
	-I.							\
	-I$(srcdir) 				\
	$(GNOME_APPLETS_CFLAGS)		\
	$(LIBGLADE_CFLAGS)			\
	$(GST_CFLAGS)				\
	$(GNOME_LIBS2_CFLAGS)		\
	$(GLIB_CFLAGS)				\
	-I$(includedir) 				

LDADD =							\
	$(GLIB_LIBS)				\
	$(GNOME_LIBS2_LIBS)			\
	$(GNOME_APPLETS_LIBS)		\
	$(LIBGLADE_LIBS)			\
	$(GST_LIBS)					

bin_PROGRAMS = alarm-applet
alarm_applet_SOURCES = alarm-applet.c alarm-applet.h

serverdir       = $(libdir)/bonobo/servers
server_in_files = GNOME_AlarmApplet.server.in
server_DATA     = $(server_in_files:.server.in=.server)

$(server_DATA): $(server_in_files)
	sed -e "s|\@BINDIR\@|$(bindir)|" $< > $@

uidir = $(gladedir)
ui_DATA = ui/alarm-applet.glade

schemasdir   = @GCONF_SCHEMA_FILE_DIR@
schemas_in_files = alarm.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

CLEANFILES = $(server_DATA)

# don't do this when building packages
install-schemas: $(schemas_DATA)
	if test -z "$(DESTDIR)"; then \
		GCONF_CONFIG_SOURCE=`$(GCONFTOOL) --get-default-source` \
		$(GCONFTOOL) --makefile-install-rule \
		$(schemas_DATA); \
	fi

install-data-local: install-schemas

#@INTLTOOL_SCHEMAS_RULE@

#@INTLTOOL_SERVER_RULE@

#EXTRA_DIST = \
#	     $(schemas_in_files)