include $(top_srcdir)/Makefile.decl

SUBDIRS = . tests

iconsdir = $(datadir)/icons
soundsdir= $(datadir)/sounds
gladedir = $(pkgdatadir)/glade

INCLUDES =						\
	-I.							\
	-I$(srcdir) 				\
	$(GNOME_APPLETS_CFLAGS)		\
	$(LIBGLADE_CFLAGS)			\
	$(GST_CFLAGS)				\
	$(GNOME_LIBS2_CFLAGS)		\
	$(GLIB_CFLAGS)				\
	$(LIBNOTIFY_CFLAGS)			\
	-I$(includedir) 			

AM_CFLAGS =								\
	-DALARM_DATADIR="\"$(datadir)\""	\
	-DALARM_GLADEDIR="\"$(gladedir)\""	\
	-DALARM_SOUNDSDIR="\"$(soundsdir)\""

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

alarm_clock_applet_LDADD =		\
	$(GLIB_LIBS)				\
	$(GNOME_LIBS2_LIBS)			\
	$(GNOME_APPLETS_LIBS)		\
	$(LIBGLADE_LIBS)			\
	$(GST_LIBS)					\
	$(LIBNOTIFY_LIBS)

serverdir       = $(libdir)/bonobo/servers
server_in_files = AlarmClock.server.in
server_DATA     = $(server_in_files:.server.in=.server)

$(server_DATA): $(server_in_files)
	sed -e "s|\@LIBEXECDIR\@|$(libexecdir)|" $< > $@

uidir = $(gladedir)
ui_DATA = ui/alarm-clock.glade

icons =	\
	icons/alarm-clock.svg		\
	icons/alarm-clock.16.png	\
	icons/alarm-clock.22.png	\
	icons/alarm-clock.24.png	\
	icons/alarm-clock.32.png	\
	icons/alarm-clock.36.png	\
	icons/alarm-clock.48.png	\
	icons/alarm-clock.64.png	\
	icons/alarm-timer.svg		\
	icons/alarm-timer.16.png	\
	icons/alarm-timer.22.png	\
	icons/alarm-timer.24.png	\
	icons/alarm-timer.32.png	\
	icons/alarm-timer.36.png	\
	icons/alarm-timer.48.png	\
	icons/alarm-timer.64.png	\
	icons/alarm-snooze.16.png	\
	icons/alarm-snooze.22.png	\
	icons/alarm-snooze.24.png	\
	icons/alarm-snooze.32.png	\
	icons/alarm-snooze.36.png	\
	icons/alarm-snooze.48.png	\
	icons/alarm-snooze.64.png	

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-icons:
	for icon in $(icons); do \
		SIZE=`basename $$icon | cut -d. -f2`; \
		FILE=`basename $$icon | cut -d. -f1,3`; \
		if [ "$$SIZE" = "svg" ]; then \
			SIZE="scalable"; \
			FILE="$$FILE.svg"; \
		else \
			SIZE="$${SIZE}x$${SIZE}"; \
		fi; \
		mkdir -p $(DESTDIR)$(iconsdir)/hicolor/$$SIZE/apps/; \
		$(INSTALL_DATA) $(srcdir)/$$icon $(DESTDIR)$(iconsdir)/hicolor/$$SIZE/apps/$$FILE; \
	done
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated. After install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

uninstall-icons:
	for icon in $(icons); do \
		SIZE=`basename $$icon | cut -d. -f2`; \
		FILE=`basename $$icon | cut -d. -f1,3`; \
		if [ "$$SIZE" = "svg" ]; then \
			SIZE="scalable"; \
			FILE="$$FILE.svg"; \
		else \
			SIZE="$${SIZE}x$${SIZE}"; \
		fi; \
		rm -f $(DESTDIR)$(iconsdir)/hicolor/$$SIZE/apps/$$FILE; \
	done

schemadir = $(GCONF_SCHEMA_FILE_DIR)
schema_in_files = alarm-clock.schemas.in
schema_DATA = $(schema_in_files:.schemas.in=.schemas)

CLEANFILES = $(server_DATA)

#@INTLTOOL_SCHEMAS_RULE@
#@INTLTOOL_SERVER_RULE@

if GCONF_SCHEMAS_INSTALL
install-schema:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_builddir)/src/$(schema_DATA)

install-data-local: install-schema install-icons
uninstall-local: uninstall-schema uninstall-icons uninstall-data
else
install-data-local: install-icons
uninstall-local: uninstall-icons uninstall-data
endif


EXTRA_DIST += $(server_in_files)	\
			  $(schemas_DATA)		\
		      $(icons)				\
		      $(ui_DATA)			
