// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * player.h - Simple media player based on GStreamer
 *
 * Copyright (C) 2007-2008 Johannes H. Jensen <joh@pseudoberries.com>
 * Copyright (C) 2022 Tasos Sahanidis <code@tasossah.com>
 */

#ifndef PLAYER_H_
#define PLAYER_H_

#include <gst/gst.h>

G_BEGIN_DECLS

typedef enum {
    MEDIA_PLAYER_INVALID = 0,
    MEDIA_PLAYER_STOPPED,
    MEDIA_PLAYER_PLAYING,
    MEDIA_PLAYER_ERROR,
} MediaPlayerState;

typedef struct _MediaPlayer MediaPlayer;

/*
 * Callback for when the media player's state changes.
 */
typedef void (*MediaPlayerStateChangeCallback)(MediaPlayer* player, MediaPlayerState state, gpointer data);

/*
 * Callback for when an error occurs in the media player.
 * The error details is put in the error argument. This value
 * should _never_ be freed in the callback!
 */
typedef void (*MediaPlayerErrorHandler)(MediaPlayer* player, GError* error, gpointer data);

struct _MediaPlayer {
    GstElement* player;
    gboolean loop;
    MediaPlayerState state;

    guint watch_id;

    MediaPlayerStateChangeCallback state_changed;
    MediaPlayerErrorHandler error_handler;

    gpointer state_changed_data;
    gpointer error_handler_data;
};

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

MediaPlayer* media_player_new(const gchar* uri, gboolean loop, MediaPlayerStateChangeCallback state_callback, gpointer data, MediaPlayerErrorHandler error_handler, gpointer error_data);

/**
 * Free a media player.
 */
void media_player_free(MediaPlayer* player);

/**
 * Set the uri of player.
 */
void media_player_set_uri(MediaPlayer* player, const gchar* uri);

/**
 * Get the uri of player.
 *
 * Free with g_free()
 */
gchar* media_player_get_uri(MediaPlayer* player);

/**
 * Set media player state.
 */
void media_player_set_state(MediaPlayer* player, MediaPlayerState state);

/**
 * Start media player
 */
void media_player_start(MediaPlayer* player);

/**
 * Stop player
 */
void media_player_stop(MediaPlayer* player);

G_END_DECLS

#endif /*PLAYER_H_*/
