INCLUDES =						\
	-I.							\
	-I$(srcdir) 				\
	$(GNOME_APPLETS_CFLAGS)		\
	$(LIBGLADE_CFLAGS)			\
	$(GST_CFLAGS)				\
	$(GNOME_LIBS2_CFLAGS)		\
	$(GLIB_CFLAGS)				\
	-I$(includedir) 			

bin_PROGRAMS = alarm-applet test_player

test_player_SOURCES = player.c player.h test_player.c
test_player_LDADD =				\
	$(GLIB_LIBS)				\
	$(GST_LIBS)

alarm_applet_SOURCES = alarm-applet.c alarm-applet.h player.c player.h
alarm_applet_LDADD =			\
	$(GLIB_LIBS)				\
	$(GNOME_LIBS2_LIBS)			\
	$(GNOME_APPLETS_LIBS)		\
	$(LIBGLADE_LIBS)			\
	$(GST_LIBS)	

serverdir       = $(libdir)/bonobo/servers
server_in_files = GNOME_AlarmApplet.server.in
server_DATA     = $(server_in_files:.server.in=.server)

$(server_DATA): $(server_in_files)
	sed -e "s|\@BINDIR\@|$(bindir)|" $< > $@

uidir = $(gladedir)
ui_DATA = ui/alarm-applet.glade

icons =	\
	alarm-clock.svg		\
	alarm-clock.16.png	\
	alarm-clock.22.png	\
	alarm-clock.24.png	\
	alarm-clock.32.png	\
	alarm-clock.36.png	\
	alarm-clock.48.png	\
	alarm-clock.64.png

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-icons:
	for icon in $(icons); do \
		SIZE=`echo $$icon | cut -d. -f2`; \
		FILE=`echo $$icon | cut -d. -f1,3`; \
		if [ "$$SIZE" = "svg" ]; then \
			SIZE="scalable"; \
			FILE="$$FILE.svg"; \
		else \
			SIZE="$${SIZE}x$${SIZE}"; \
		fi; \
		mkdir -p $(DESTDIR)$(iconsdir)/hicolor/$$SIZE/apps/; \
		$(INSTALL_DATA) $(srcdir)/icons/$$icon $(DESTDIR)$(iconsdir)/hicolor/$$SIZE/apps/$$FILE; \
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
		SIZE=`echo $$icon | cut -d. -f2`; \
		FILE=`echo $$icon | cut -d. -f1,3`; \
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
	fi

install-data-local: install-schemas install-icons

uninstall-local: uninstall-icons

#@INTLTOOL_SCHEMAS_RULE@

#@INTLTOOL_SERVER_RULE@

#EXTRA_DIST = \
#	     $(schemas_in_files)