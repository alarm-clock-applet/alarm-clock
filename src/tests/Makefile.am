INCLUDES =						\
	-I.							\
	-I$(srcdir) 				\
	$(GNOME_APPLETS_CFLAGS)		\
	$(LIBGLADE_CFLAGS)			\
	$(GST_CFLAGS)				\
	$(GNOME_LIBS2_CFLAGS)		\
	$(GLIB_CFLAGS)				\
	-I$(includedir) 			\
	-I$(top_srcdir)/src

bin_PROGRAMS = test_player

test_player_SOURCES = $(top_srcdir)/src/player.c $(top_srcdir)/src/player.h test_player.c
test_player_LDADD =				\
	$(GLIB_LIBS)				\
	$(GST_LIBS)