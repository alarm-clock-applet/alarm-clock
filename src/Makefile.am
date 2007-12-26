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

bin_PROGRAMS = alarm-applet

alarm_applet_SOURCES = alarm-applet.c alarm-applet.h \
					   player.c player.h			 \
					   util.c util.h				 \
					   list-entry.c list-entry.h	 \
					   alarm-gconf.c alarm-gconf.h	 \
					   ui.c ui.h					 \
					   prefs.c prefs.h

alarm_applet_LDADD =			\
	$(GLIB_LIBS)				\
	$(GNOME_LIBS2_LIBS)			\
	$(GNOME_APPLETS_LIBS)		\
	$(LIBGLADE_LIBS)			\
	$(GST_LIBS)					\
	$(LIBNOTIFY_LIBS)

serverdir       = $(libdir)/bonobo/servers
server_in_files = GNOME_AlarmApplet.server.in
server_DATA     = $(server_in_files:.server.in=.server)

$(server_DATA): $(server_in_files)
	sed -e "s|\@BINDIR\@|$(bindir)|" $< > $@

uidir = $(gladedir)
ui_DATA = ui/alarm-applet.glade

icons =	\
	icons/alarm-clock.svg		\
	icons/alarm-clock.16.png	\
	icons/alarm-clock.22.png	\
	icons/alarm-clock.24.png	\
	icons/alarm-clock.32.png	\
	icons/alarm-clock.36.png	\
	icons/alarm-clock.48.png	\
	icons/alarm-clock.64.png

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
		killall -SIGTERM gconfd-2; \
	fi

uninstall-schemas: $(schemas_DATA)
	if test -z "$(DESTDIR)"; then \
		GCONF_CONFIG_SOURCE=`$(GCONFTOOL) --get-default-source` \
		$(GCONFTOOL) --makefile-uninstall-rule \
		$(schemas_DATA); \
		killall -SIGTERM gconfd-2; \
	fi

uninstall-data:
	rm -rf $(pkgdatadir)

install-data-local: install-schemas install-icons

uninstall-local: uninstall-schemas uninstall-icons uninstall-data

#@INTLTOOL_SCHEMAS_RULE@

#@INTLTOOL_SERVER_RULE@

EXTRA_DIST = $(server_in_files)	\
		     $(schemas_DATA)	\
		     $(icons)			\
		     $(ui_DATA)			\
		     tests/*.c			\
		     tests/Makefile.am	