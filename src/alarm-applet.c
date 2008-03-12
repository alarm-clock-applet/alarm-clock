#include "alarm-applet.h"

#include "alarm.h"

/*
 * DEFINTIIONS {{
 */

gchar *supported_sound_mime_types [] = { "audio", "video", "application/ogg", NULL };
GHashTable *app_command_map = NULL;

/*
 * }} DEFINTIIONS
 */


/**
 * Player state changed
 */
static void
player_state_cb (MediaPlayer *player, MediaPlayerState state, AlarmApplet *applet)
{
	if (state == MEDIA_PLAYER_STOPPED) {
		if (player == applet->player)
			applet->player = NULL;
		else if (player == applet->preview_player)
			applet->preview_player = NULL;
		
		g_debug ("Freeing media player %p", player);
		
		media_player_free (player);
	}
}

static void
player_preview_state_cb (MediaPlayer *player, MediaPlayerState state, AlarmApplet *applet)
{
	const gchar *stock;
	
	if (state == MEDIA_PLAYER_PLAYING)
		stock = "gtk-media-stop";
	else
		stock = "gtk-media-play";
	
	// Set stock
	gtk_button_set_label (GTK_BUTTON (applet->pref_notify_sound_preview), stock);
	
	player_state_cb (player, state, applet);
}

/**
 * Play sound via gstreamer
 */
void
player_start (AlarmApplet *applet)
{
	if (applet->player == NULL)
		applet->player = media_player_new (get_sound_file (applet),
										   applet->notify_sound_loop,
										   player_state_cb, applet,
										   media_player_error_cb, applet);
	
	media_player_set_uri (applet->player, get_sound_file (applet));
	
	media_player_start (applet->player);
	
	g_debug ("player_start...");
}

void
player_preview_start (AlarmApplet *applet)
{
	if (applet->preview_player == NULL)
		applet->preview_player = media_player_new (get_sound_file (applet),
												   FALSE,
												   player_preview_state_cb, applet,
												   media_player_error_cb, applet);
	
	media_player_set_uri (applet->preview_player, get_sound_file (applet));
	
	media_player_start (applet->preview_player);
	
	g_debug ("preview_start...");
}

/*
 * }} Media player
 */





/*
 * TIMER {{
 */

void
trigger_alarm (AlarmApplet *applet)
{
	g_debug("ALARM: %s", applet->alarm_message);
	
	alarm_gconf_set_started (applet, FALSE);
	
	applet->alarm_triggered = TRUE;
	
	switch (applet->notify_type) {
	case NOTIFY_SOUND:
		// Start sound playback
		player_start (applet);
		break;
	case NOTIFY_COMMAND:
		// Start app
		g_debug ("START APP");
		command_run (applet->notify_command);
		break;
	default:
		g_debug ("NOTIFICATION TYPE %d Not yet implemented.", applet->notify_type);
	}
	
	update_tooltip (applet);
	
#ifdef HAVE_LIBNOTIFY
	if (applet->notify_bubble) {
		display_notification (applet);
	}
#endif
}

void
clear_alarm (AlarmApplet *applet)
{
	alarm_gconf_set_started (applet, FALSE);
	applet->alarm_triggered = FALSE;
	
	timer_remove (applet);
	update_label (applet);
	update_tooltip (applet);
	
	// Stop playback if present.
	if (applet->player && applet->player->state == MEDIA_PLAYER_PLAYING)
		media_player_stop (applet->player);
	
	// Close notification if present.
#ifdef HAVE_LIBNOTIFY
	if (applet->notify) {
		close_notification (applet);
	}
#endif
}

static gboolean
timer_update (AlarmApplet *applet)
{
	time_t now;
	
	time (&now);
	
	if (now >= applet->alarm_time) {
		trigger_alarm (applet);
		
		// Remove callback
		return FALSE;
	} else if (applet->alarm_time - now <= 10) {
		g_debug ("%2d...", (int)(applet->alarm_time - now));
	}
	
	if (applet->show_label && applet->label_type == LABEL_TYPE_REMAIN) {
		update_label (applet);
	}
	
	// Keep callback
	return TRUE;
}

void
timer_start (AlarmApplet *applet)
{
	g_debug ("timer_start");
	
	// Remove old timer, if any
	timer_remove (applet);
	
	applet->timer_id = g_timeout_add_seconds (1, (GSourceFunc) timer_update, applet);
}

void
timer_remove (AlarmApplet *applet)
{
	if (applet->timer_id > 0) {
		g_source_remove (applet->timer_id);
		
		applet->timer_id = 0;
	}
}

/*
 * }} TIMER
 */




gboolean
set_sound_file (AlarmApplet *applet, const gchar *uri)
{
	AlarmListEntry *item, *p;
	GnomeVFSResult result;
	gchar *mime;
	GList *l;
	guint pos, i;
	gboolean valid;
	
	if (uri == NULL || g_utf8_strlen(uri, 1) == 0)
		return FALSE;
	
	g_debug ("set_sound_file (%s)", uri);
	
	item = alarm_list_entry_new_file (uri, &result, &mime);
	
	if (item == NULL) {
		g_debug ("Invalid sound file: %s", uri);
		
		if (applet->preferences_dialog != NULL) {
			gchar *message = g_strdup_printf ("Unable to open '%s': %s.", uri, gnome_vfs_result_to_string (result));
			display_error_dialog("Invalid Sound File", message, GTK_WINDOW (applet->preferences_dialog));
			g_free(message);
		}
		
		g_free(mime);
		return FALSE;
	}
	
	valid = FALSE;
	for (i = 0; supported_sound_mime_types[i] != NULL; i++) {
		if (strstr (mime, supported_sound_mime_types[i]) != NULL) {
			// MATCH
			//g_debug (" [ MATCH ]");
			valid = TRUE;
			break;
		}
	}
				
	if (!valid) {
		g_critical ("Invalid sound file '%s': MIME-type not supported: %s", uri, mime);
		
		if (applet->preferences_dialog != NULL) {
			gchar *message = g_strdup_printf ("The file '%s' is not a valid media file.", uri);
			display_error_dialog("Invalid Sound File", message, GTK_WINDOW (applet->preferences_dialog));
			g_free(message);
		}
		
		g_free (mime);
		
		return FALSE;
	}
	
	g_free (mime);
	
	// See if it's one of the already loaded files
	for (l = applet->sounds, pos = 0; l != NULL; l = l->next, pos++) {
		p = (AlarmListEntry *) l->data;
		g_debug ("CHECK %s == %s", p->data, uri);
		if (strcmp (p->data, uri) == 0) {
			g_debug ("set_sound_file FOUND at pos %d", pos);
			break;
		}
	}
	
	g_debug ("After search, pos = %d vs length %d", pos, g_list_length (applet->sounds));
	
	// If not, add it to the list
	if (pos == g_list_length (applet->sounds)) {
		g_debug ("Adding to list: %s %s %s", item->name, item->data, item->icon);
		applet->sounds = g_list_append (applet->sounds, item);
	}
	
	applet->sound_pos = pos;
	
	return TRUE;
}

const gchar *
get_sound_file (AlarmApplet *applet)
{
	AlarmListEntry *e;
	e = (AlarmListEntry *) g_list_nth_data (applet->sounds, applet->sound_pos);
	
	g_debug ("get_sound_file returning %p", e);
	
	return (const gchar *)e->data;
}

// Load sounds into list
void
load_sounds_list (AlarmApplet *applet)
{
	Alarm *alarm;
	AlarmListEntry *entry;
	GList *l, *l2;
	gboolean found;
	
	g_assert (applet->alarms);
	
	// Free old list
	if (applet->sounds != NULL)
		alarm_list_entry_list_free (&(applet->sounds));
	
	// Load stock sounds
	applet->sounds = alarm_list_entry_list_new ("file://" ALARM_SOUNDS_DIR,
												supported_sound_mime_types);
	
	// Load custom sounds from alarms
	for (l = applet->alarms; l != NULL; l = l->next) {
		alarm = ALARM (l->data);
		found = FALSE;
		for (l2 = applet->sounds; l2 != NULL; l2 = l2->next) {
			entry = (AlarmListEntry *)l2->data;
			if (strcmp (alarm->sound_file, entry->data) == 0) {
				// FOUND
				found = TRUE;
				break;
			}
		}
		
		if (!found) {
			// Add to list
			entry = alarm_list_entry_new_file (alarm->sound_file, NULL, NULL);
			if (entry) {
				applet->sounds = g_list_append (applet->sounds, entry);
			}
		}
	}
}

// Notify callback for changes to an alarm's sound_file
void
alarm_sound_file_changed (GObject *object, 
						  GParamSpec *param,
						  gpointer data)
{
	Alarm *alarm		= ALARM (object);
	AlarmApplet *applet = (AlarmApplet *)data;
	
	g_debug ("alarm_sound_file_changed: #%d", alarm->id);
	
	// Reload sounds list
	load_sounds_list (applet);
}


/*
 * INIT {{
 */

static gboolean
alarm_applet_factory (PanelApplet *panelapplet,
					  const gchar *iid,
					  gpointer data)
{
	AlarmApplet *applet;
	AlarmListEntry *item;
	gchar *tmp;
	
	if (strcmp (iid, "OAFIID:GNOME_AlarmApplet") != 0)
		return FALSE;
	
	
	/* Initialize applet struct */
	applet = g_new(AlarmApplet, 1);
	applet->alarms = NULL;
	applet->alarm_time = 0;
	applet->alarm_message = NULL;
	applet->started = FALSE;
	applet->alarm_triggered = FALSE;
	applet->timer_id = 0;
	applet->player = NULL;
	applet->preview_player = NULL;
	applet->sounds = NULL;
	applet->sound_pos = 0;
	applet->apps = NULL;
	
	applet->parent = panelapplet;
	
	applet->set_alarm_dialog = NULL;
	applet->list_alarms_dialog = NULL;
	applet->list_alarms_store = NULL;
	applet->preferences_dialog = NULL;
	
	/* Preferences (defaults). 
	 * ...gconf_get_string can return NULL if the key is not found. We can't
	 * assume the schema provides the default values for strings. */
	applet->show_label = TRUE;
	applet->label_type = LABEL_TYPE_ALARM;
	applet->notify_type = NOTIFY_SOUND;
	applet->notify_sound_loop = TRUE;
	applet->notify_bubble = TRUE;
	
	/* Prepare */
	panel_applet_add_preferences (applet->parent, ALARM_SCHEMA_DIR, NULL);
	panel_applet_set_flags (PANEL_APPLET (panelapplet), PANEL_APPLET_EXPAND_MINOR);
	
	// Load sounds list
	//load_stock_sounds_list (applet);
	
	// Load applications list
	load_app_list (applet);
	
	/* Set up gconf handlers */
	setup_gconf (applet);
	
	/* Load gconf values */
	load_gconf (applet);
	
	/* Load alarms */
	applet->gconf_dir = panel_applet_get_preferences_key(applet->parent);
	applet->alarms = alarm_get_list (applet->gconf_dir);
	
	/* Load sounds from alarms */
	load_sounds_list (applet);
	
	/* Connect sound_file notify callback to all alarms */
	alarm_signal_connect_list (applet->alarms, "notify::sound-file", 
							   G_CALLBACK (alarm_sound_file_changed), applet);
	
	/* Set up properties menu */
	menu_setup(applet);
	
	/* Set up applet */
	ui_setup (applet);
	
	
	if (applet->started) {
		// Start the timer!
		update_label (applet);
		update_tooltip (applet);
		timer_start (applet);
	}
	
	g_debug ("GLADE DIR: %s", GNOME_GLADEDIR);
	
	return TRUE;
}

PANEL_APPLET_BONOBO_FACTORY ("OAFIID:GNOME_AlarmApplet_Factory",
                             PANEL_TYPE_APPLET,
                             "alarm_applet",
                             VERSION,
                             alarm_applet_factory,
                             NULL);


/*
 * }} INIT
 */
