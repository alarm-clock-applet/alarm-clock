include $(top_srcdir)/Makefile.decl

AM_CFLAGS = -DALARM_CLOCK_DATADIR=\"$(datadir)\"        \
	        -DALARM_CLOCK_PKGDATADIR=\"$(pkgdatadir)\"

INCLUDES =						\
    -I$(top_srcdir)             \
	-I$(top_srcdir)/src			\
	@BASE_CFLAGS@               \
	@WARN_CFLAGS@               \
	$(AM_CFLAGS)                

LIBS = @BASE_LIBS@

check_PROGRAMS = $(TEST_PROGS) test_player test_list_entry test_util glade_view test_gconf_recursive

TEST_PROGS += test_alarm

test_player_SOURCES = $(top_srcdir)/src/player.c $(top_srcdir)/src/player.h test_player.c
test_player_CFLAGS = @GSTREAMER_CFLAGS@
test_player_LDADD = @GSTREAMER_LIBS@

test_list_entry_SOURCES = $(top_srcdir)/src/list-entry.c	\
						  $(top_srcdir)/src/list-entry.h	\
						  $(top_srcdir)/src/util.c			\
						  test_list_entry.c
test_list_entry_CFLAGS = @GTK_CFLAGS@
test_list_entry_LDADD = @GTK_LIBS@

test_util_SOURCES = $(top_srcdir)/src/util.c $(top_srcdir)/src/util.h test_util.c


test_alarm_SOURCES = 			\
	$(top_srcdir)/src/alarm.c	\
	$(top_srcdir)/src/player.c	\
	test_alarm.c
	
test_alarm_CFLAGS = @GNOME_CFLAGS@ @GSTREAMER_CFLAGS@
test_alarm_LDADD = @GNOME_LIBS@ @GSTREAMER_LIBS@
	
test_alarm_cpp:
	cpp $(INCLUDES) test_alarm.c

glade_view_SOURCES = glade_view.c
glade_view_LDADD =				\
	$(GLIB_LIBS)				\
	$(LIBGLADE_LIBS)			
	
test_gconf_recursive_SOURCES = test_gconf_recursive.c
test_gconf_recursive_LDADD =	\
	$(GLIB_LIBS)				\
	$(GCONF_LIBS)
