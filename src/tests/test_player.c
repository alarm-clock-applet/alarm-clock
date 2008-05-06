/*
 * test_player.c -- Test utility for the MediaPlayer system.
 * 
 * Copyright (C) 2007-2008 Johannes H. Jensen <joh@deworks.net>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 * Authors:
 * 		Johannes H. Jensen <joh@deworks.net>
 */

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
