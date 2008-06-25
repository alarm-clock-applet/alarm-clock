/*
 * player.c - Simple media player based on GStreamer
 * 
 * Copyright (C) 2007-2008 Johannes H. Jensen <joh@pseudoberries.com>
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
 * 		Johannes H. Jensen <joh@pseudoberries.com>
 */

#include <gst/gst.h>

#include "player.h"

/**
 * Create a new media player.
 * 
 * @uri				The file to play.
 * @loop			Wether to loop or not.
 * @state_callback	An optional #MediaPlayerStateChangeCallback which will be
 * 					notified when the state of the player changes.
 * @data			Data for the state_callback
 * @error_handler	An optional #MediaPlayerErrorHandler which will be notified
 * 					if an error occurs.
 * @error_data		Data for the error_handler.
 */
MediaPlayer *
media_player_new (const gchar *uri, gboolean loop,
				  MediaPlayerStateChangeCallback state_callback, gpointer data,
				  MediaPlayerErrorHandler error_handler, gpointer error_data)
{
	MediaPlayer *player;
	GstElement *audiosink, *videosink;
	
	// Initialize struct
	player = g_new (MediaPlayer, 1);
	
	player->loop	 = loop;
	player->state	 = MEDIA_PLAYER_STOPPED;
	player->watch_id = 0;
	
	player->state_changed 		= state_callback;
	player->state_changed_data	= data;
	player->error_handler		= error_handler;
	player->error_handler_data	= error_data;
	
	// Initialize GStreamer
	gst_init (NULL, NULL);
	
	/* Set up player */
	player->player	= gst_element_factory_make ("playbin", "player");
	audiosink 		= gst_element_factory_make ("gconfaudiosink", "player-audiosink");
	videosink 		= gst_element_factory_make ("gconfvideosink", "player-videosink");
	
	if (!player->player || !audiosink || !videosink) {
		g_critical ("Could not create player.");
		return NULL;
	}
	
	// Set uri and sinks
	g_object_set (player->player,
				  "uri", uri,
				  "audio-sink", audiosink,
				  "video-sink", videosink,
				  NULL);
	
	return player;
}

/**
 * Free a media player.
 */
void
media_player_free (MediaPlayer *player)
{
	if (player->player)
		gst_object_unref (GST_OBJECT (player->player));
	
	g_free (player);
}

/**
 * Set the uri of player.
 */
void
media_player_set_uri (MediaPlayer *player, const gchar *uri)
{
	g_object_set (player->player, "uri", uri, NULL);
}

/**
 * Get the uri of player.
 * 
 * Free with g_free()
 */
gchar *
media_player_get_uri (MediaPlayer *player)
{
	gchar *uri;
	
	g_object_get (player->player, "uri", &uri, NULL);
	
	return uri;
}

/**
 * Set media player state.
 */
void
media_player_set_state (MediaPlayer *player, MediaPlayerState state)
{
	MediaPlayerState old = player->state;
	
	player->state = state;
	
	// Notify state change handler
	if (old != state && player->state_changed)
		player->state_changed(player, player->state, player->state_changed_data);
}


/**
 * Check for errors & call error handler
 */
static gboolean
media_player_bus_check_errors (MediaPlayer *player, GstMessage *message)
{
//	g_debug ("Got %s message\n", GST_MESSAGE_TYPE_NAME (message));
	
	switch (GST_MESSAGE_TYPE (message)) {
	case GST_MESSAGE_ERROR: {
		GError *err;
		gchar *debug;
		
		gst_message_parse_error (message, &err, &debug);
		
		if (player->error_handler)
			player->error_handler (player, err, player->error_handler_data);
		
		g_error_free (err);
		g_free (debug);
		
		return FALSE;
		break;
	}
	default:
		break;
	}
	
	// No errors
	return TRUE;
}

/**
 * GST bus callback.
 */
static gboolean
media_player_bus_cb (GstBus     *bus,
					 GstMessage *message,
					 MediaPlayer *player)
{
//	g_debug ("Got %s message\n", GST_MESSAGE_TYPE_NAME (message));
	
	if (media_player_bus_check_errors (player, message)) {
		if (GST_MESSAGE_TYPE (message) == GST_MESSAGE_EOS) {
			// End of Stream, do we loop?
			if (!player->loop) {
				media_player_stop (player);
				return FALSE;
			}
			
			// Loop
			gst_element_set_state (player->player, GST_STATE_READY);
			gst_element_set_state (player->player, GST_STATE_PLAYING);
			
			media_player_set_state (player, MEDIA_PLAYER_PLAYING);
		}
		
		// Keep notifying us
		return TRUE;
	}
	
	// There were errors
	media_player_stop (player);
	
	return FALSE;
}

/**
 * Start media player
 */
void
media_player_start (MediaPlayer *player)
{
	GstBus *bus;
	
	// Attach bus watcher
	bus = gst_pipeline_get_bus (GST_PIPELINE (player->player));
	player->watch_id = gst_bus_add_watch (bus, (GstBusFunc) media_player_bus_cb, player);
	gst_object_unref (bus);
	
	gst_element_set_state (player->player, GST_STATE_PLAYING);
	media_player_set_state (player, MEDIA_PLAYER_PLAYING);
}

/**
 * Stop player
 */
void
media_player_stop (MediaPlayer *player)
{
	if (player->watch_id) {
		g_source_remove (player->watch_id);
		
		player->watch_id = 0;
	}
	
	if (player->player != NULL) {
		gst_element_set_state (player->player, GST_STATE_NULL);
	}
	
	media_player_set_state (player, MEDIA_PLAYER_STOPPED);
}

/*
 * }} Media player
 */
