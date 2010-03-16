include $(top_srcdir)/Makefile.decl

iconsdir = $(datadir)/icons
icons = 			\
	alarm-clock.svg		\
	alarm-clock.16.png	\
	alarm-clock.22.png	\
	alarm-clock.24.png	\
	alarm-clock.32.png	\
	alarm-clock.36.png	\
	alarm-clock.48.png	\
	alarm-clock.64.png	\
	alarm-timer.svg		\
	alarm-timer.16.png	\
	alarm-timer.22.png	\
	alarm-timer.24.png	\
	alarm-timer.32.png	\
	alarm-timer.36.png	\
	alarm-timer.48.png	\
	alarm-timer.64.png	\
	alarm-snooze.16.png	\
	alarm-snooze.22.png	\
	alarm-snooze.24.png	\
	alarm-snooze.32.png	\
	alarm-snooze.36.png	\
	alarm-snooze.48.png	\
	alarm-snooze.64.png	

EXTRA_DIST += $(icons)


gtk_update_icon_cache = gtk-update-icon-cache -f -t $(iconsdir)/hicolor

update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

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

install-data-local: install-icons update-icon-cache

uninstall-local: uninstall-icons update-icon-cache
