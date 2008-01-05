INCLUDES =						\
	-I.							\
	-I$(srcdir) 				\
	$(GNOME_APPLETS_CFLAGS)		\
	$(LIBGLADE_CFLAGS)			\
	$(GST_CFLAGS)				\
	$(GNOME_LIBS2_CFLAGS)		\
	$(GLIB_CFLAGS)				\
	$(GCONF_CFLAGS)				\
	-I$(includedir) 			\
	-I$(top_srcdir)/src

bin_PROGRAMS = test_player test_list_entry test_util test_alarm

test_player_SOURCES = $(top_srcdir)/src/player.c $(top_srcdir)/src/player.h test_player.c
test_player_LDADD =				\
	$(GLIB_LIBS)				\
	$(GST_LIBS)
	
test_list_entry_SOURCES = $(top_srcdir)/src/list-entry.c	\
						  $(top_srcdir)/src/list-entry.h	\
						  $(top_srcdir)/src/util.c			\
						  test_list_entry.c

test_list_entry_LDADD =			\
	$(GLIB_LIBS)				\
	$(GNOME_LIBS2_LIBS)			\
	$(GNOME_APPLETS_LIBS)		

test_util_SOURCES = $(top_srcdir)/src/util.c $(top_srcdir)/src/util.h test_util.c
test_util_LDADD =				\
	$(GLIB_LIBS)				

test_alarm_SOURCES = 			\
	$(top_srcdir)/src/alarm.h	\
	$(top_srcdir)/src/alarm.c	\
	test_alarm.c
	
test_alarm_LDADD =				\
	$(GLIB_LIBS)				\
	$(GCONF_LIBS)
	
test_alarm_cpp:
	cpp $(INCLUDES) ../alarm.c