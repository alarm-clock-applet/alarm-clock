#include <glib.h>

#include "player.h"

void state_changed (MediaPlayer *player, MediaPlayerState state, gchar *data)
{
	g_debug ("State changed to %s [%d], data is '%s'", 
				(state == MEDIA_PLAYER_PLAYING) ? "PLAYING" : "STOPPED",
				state, data);
}

void error_handler (MediaPlayer *player, GError *error, gchar *data)
{
	g_debug ("Error occured: %s, data is '%s'", error->message, data);
}

int main (int argc, char **argv)
{
	if (argc < 2) {
		g_print ("Usage: %s <uri>\n", argv[0]);
		return 1;
	}
	
	MediaPlayer *player;
	GMainLoop *loop;
	
	player = media_player_new(argv[1], FALSE, 
							  state_changed, "test data", 
							  error_handler, "test error");
	
	media_player_start (player);
	
	loop = g_main_loop_new (g_main_context_default(), TRUE);
	
	g_main_loop_run (loop);
	
	media_player_free(player);
	
	return 0;
}
