include $(top_srcdir)/Makefile.decl

soundsdir= $(datadir)/sounds
gladedir = $(pkgdatadir)/glade

SUBDIRS = tests

AM_CFLAGS =	                        \
	-DALARM_CLOCK_DATADIR=\"$(datadir)\"            \
	-DALARM_CLOCK_PKGDATADIR=\"$(pkgdatadir)\"      \
	@BASE_CFLAGS@ @GTK_CFLAGS@ @GSTREAMER_CFLAGS@   \
	@GNOME_CFLAGS@ @APP_INDICATOR_CFLAGS@ @WARN_CFLAGS@ \
 	-Wno-format-y2k

LDADD =		\
	@BASE_LIBS@ @GTK_LIBS@ @GSTREAMER_LIBS@         \
	@GNOME_LIBS@ @APP_INDICATOR_LIBS@

#
# alarm-clock-applet binary
#

bin_PROGRAMS = alarm-clock-applet

alarm_clock_applet_SOURCES = alarm-applet.c alarm-applet.h          	\
							 player.c player.h				            \
						     util.c util.h				            	\
						     list-entry.c list-entry.h		            \
						     alarm.c alarm.h                            \
						     alarm-gconf.c alarm-gconf.h            	\
						     ui.c ui.h					            	\
						     alarm-actions.c alarm-actions.h            \
						     alarm-list-window.c alarm-list-window.h	\
						     alarm-settings.c alarm-settings.h	    	\
						     prefs.c prefs.h                            



