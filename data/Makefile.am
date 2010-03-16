SUBDIRS = icons

uidir = $(pkgdatadir)
ui_DATA = alarm-clock.ui


desktopdir   = $(datadir)/applications
desktop_in_files = alarm-clock-applet.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

schemasdir = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_files = alarm-clock.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

@INTLTOOL_SCHEMAS_RULE@

if GCONF_SCHEMAS_INSTALL
install-data-local:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
	$(GCONFTOOL) --makefile-install-rule $(schemas_DATA)

uninstall-local:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
	$(GCONFTOOL) --makefile-uninstall-rule $(schemas_DATA)
endif


EXTRA_DIST = $(ui_DATA) $(desktop_in_files) $(schema_in_files)

CLEANFILES = $(desktop_DATA) $(schemas_DATA)

