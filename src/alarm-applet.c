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
	
	//g_assert (applet->alarms);
	
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

static gchar *
gnome_da_xml_get_string (const xmlNode *parent, const gchar *val_name)
{
    const gchar * const *sys_langs;
    xmlChar *node_lang;
    xmlNode *element;
    gchar *ret_val = NULL;
    xmlChar *xml_val_name;
    gint len;
    gint i;

    g_return_val_if_fail (parent != NULL, ret_val);
    g_return_val_if_fail (parent->children != NULL, ret_val);
    g_return_val_if_fail (val_name != NULL, ret_val);

#if GLIB_CHECK_VERSION (2, 6, 0)
    sys_langs = g_get_language_names ();
#endif

    xml_val_name = xmlCharStrdup (val_name);
    len = xmlStrlen (xml_val_name);

    for (element = parent->children; element != NULL; element = element->next) {
		if (!xmlStrncmp (element->name, xml_val_name, len)) {
		    node_lang = xmlNodeGetLang (element);
	
		    if (node_lang == NULL) {
		    	ret_val = (gchar *) xmlNodeGetContent (element);
		    } else {
				for (i = 0; sys_langs[i] != NULL; i++) {
				    if (!strcmp (sys_langs[i], node_lang)) {
						ret_val = (gchar *) xmlNodeGetContent (element);
						/* since sys_langs is sorted from most desirable to
						 * least desirable, exit at first match
						 */
						break;
				    }
				}
		    }
		    xmlFree (node_lang);
		}
    }

    xmlFree (xml_val_name);
    return ret_val;
}

static const gchar *
get_app_command (const gchar *app)
{
	// TODO: Shouldn't be a global variable
	if (app_command_map == NULL) {
		app_command_map = g_hash_table_new (g_str_hash, g_str_equal);
		
		/* `rhythmbox-client --play' doesn't actually start playing unless
		 * Rhythmbox is already running. Sounds like a Bug. */
		g_hash_table_insert (app_command_map,
							 "rhythmbox", "rhythmbox-client --play");
		
		g_hash_table_insert (app_command_map,
							 "banshee", "banshee --play");
		
		// Note that totem should already be open with a file for this to work.
		g_hash_table_insert (app_command_map,
							 "totem", "totem --play");
		
		// Muine crashes and doesn't seem to have any play command
		/*g_hash_table_insert (app_command_map,
							 "muine", "muine");*/
	}
	
	return g_hash_table_lookup (app_command_map, app);
}

// Load stock apps into list
void
load_apps_list (AlarmApplet *applet)
{
	AlarmListEntry *entry;
	gchar *filename, *name, *icon, *command;
	xmlDoc *xml_doc;
	xmlNode *root, *section, *element;
    gchar *executable;
    const gchar *tmp;
	
	if (applet->apps != NULL)
		alarm_list_entry_list_free (&(applet->apps));

	// We'll get the default media players from g-d-a.xml
	// from gnome-control-center
	filename = g_build_filename (DATADIR,
								 "gnome-control-center",
					 			 "gnome-default-applications.xml",
					 			 NULL);
	
	if (!g_file_test (filename, G_FILE_TEST_EXISTS)) {
		g_critical ("Could not find %s.", filename);
		return;
	}

    xml_doc = xmlParseFile (filename);

    if (!xml_doc) {
    	g_warning ("Could not load %s.", filename);
    	return;
    }

    root = xmlDocGetRootElement (xml_doc);

	for (section = root->children; section != NULL; section = section->next) {
		if (!xmlStrncmp (section->name, "media-players", 13)) {
		    for (element = section->children; element != NULL; element = element->next) {
				if (!xmlStrncmp (element->name, "media-player", 12)) {
				    executable = gnome_da_xml_get_string (element, "executable");
				    if (is_executable_valid (executable)) {
				    	name = gnome_da_xml_get_string (element, "name");
				    	icon = gnome_da_xml_get_string (element, "icon-name");
				    	
				    	// Lookup executable in app command map
				    	tmp = get_app_command (executable);
				    	if (tmp)
				    		command = g_strdup (tmp);
				    	else {
				    		// Fall back to command specified in XML
				    		command = gnome_da_xml_get_string (element, "command");
				    	}
						
						
						
						g_debug ("LOAD-APPS: Adding '%s': %s [%s]", name, command, icon);
						
						entry = alarm_list_entry_new (name, command, icon);
						
						g_free (name);
						g_free (command);
						g_free (icon);
						
						applet->apps = g_list_append (applet->apps, entry);
				    }
				    
				    if (executable)
				    	g_free (executable);
				}
		    }
	    }
	}
	
	
	
	
	g_free (filename);
	
//	entry = alarm_list_entry_new("Rhythmbox Music Player", "rhythmbox", "rhythmbox");
//	applet->apps = g_list_append (applet->apps, entry);
}


void
alarm_applet_alarms_load (AlarmApplet *applet)
{
	if (applet->alarms != NULL) {
		GList *l;
		
		/* Free old alarm objects */
		for (l = applet->alarms; l != NULL; l = l->next) {
			g_object_unref (ALARM (l->data));
		}
		
		/* Free list */
		g_list_free (applet->alarms);
	}
	
	/* Fetch list of alarms */
	applet->alarms = alarm_get_list (applet->gconf_dir);
}

void
alarm_applet_alarms_add (AlarmApplet *applet, Alarm *alarm)
{
	applet->alarms = g_list_append (applet->alarms, alarm);
}

void
alarm_applet_alarms_remove (AlarmApplet *applet, Alarm *alarm)
{
	/*
	 * Remove from list
	 */
	applet->alarms = g_list_remove (applet->alarms, alarm);
	
	/*
	 * Clear list store. This will decrease the refcount of our alarms by 1.
	 */
	if (applet->list_alarms_store)
		gtk_list_store_clear (applet->list_alarms_store);
	
	g_debug ("alarm_applet_alarms_remove (..., %p): refcount = %d", alarm, G_OBJECT (alarm)->ref_count);
	
	/*
	 * Dereference alarm
	 */
	g_object_unref (alarm);
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
	applet->edit_alarm_dialogs = g_hash_table_new (NULL, NULL);
	
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
	//load_app_list (applet);
	
	/* Set up gconf handlers */
	setup_gconf (applet);
	
	/* Load gconf values */
	load_gconf (applet);
	
	/* Load alarms */
	applet->gconf_dir = panel_applet_get_preferences_key (applet->parent);
	alarm_applet_alarms_load (applet);
	
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
