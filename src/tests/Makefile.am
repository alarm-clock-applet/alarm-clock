include $(top_srcdir)/Makefile.decl

AM_CFLAGS =	                        				\
	-DALARM_CLOCK_DATADIR=\"$(datadir)\"            \
	-DALARM_CLOCK_PKGDATADIR=\"$(pkgdatadir)\"      \
	-I$(top_srcdir)             					\
	-I$(top_srcdir)/src								\
	@BASE_CFLAGS@ @GTK_CFLAGS@ @GSTREAMER_CFLAGS@   \
	@GNOME_CFLAGS@  @WARN_CFLAGS@			                

LIBS = @BASE_LIBS@

check_PROGRAMS = $(TEST_PROGS) test_player test_list_entry test_util \
                 test_gconf_recursive test_libnotify

TEST_PROGS += test_alarm

test_player_SOURCES = $(top_srcdir)/src/player.c $(top_srcdir)/src/player.h test_player.c
test_player_LDADD = @GSTREAMER_LIBS@

test_list_entry_SOURCES = $(top_srcdir)/src/list-entry.c	\
						  $(top_srcdir)/src/list-entry.h	\
						  $(top_srcdir)/src/util.c			\
						  test_list_entry.c
test_list_entry_LDADD = @GTK_LIBS@

test_util_SOURCES = $(top_srcdir)/src/util.c $(top_srcdir)/src/util.h test_util.c
test_util_LDADD = @GTK_LIBS@

test_alarm_SOURCES = 			\
	$(top_srcdir)/src/alarm.c	\
	$(top_srcdir)/src/player.c	\
	test_alarm.c
	
test_alarm_LDADD = @GNOME_LIBS@ @GSTREAMER_LIBS@
	
test_alarm_cpp:
	cpp $(INCLUDES) test_alarm.c
	
test_gconf_recursive_SOURCES = test_gconf_recursive.c
test_gconf_recursive_LDADD = @GNOME_LIBS@

test_libnotify_SOURCES = test_libnotify.c
test_libnotify_LDADD = @GTK_LIBS@ @GNOME_LIBS@
