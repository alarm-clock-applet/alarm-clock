#include "alarm-applet.h"

/*
 * DEFINTIIONS {{
 */

static gchar *supported_sound_mime_types [] = { "audio", "video", "application/ogg", NULL };

/*
 * }} DEFINTIIONS
 */


/**
 * Player error
 */
static void
player_error_cb (MediaPlayer *player, GError *err, AlarmApplet *applet)
{
	gchar *uri;
	
	uri = media_player_get_uri (player);
	g_critical ("Could not play '%s': %s", err->message);
	g_free (uri);
	
	if (applet->preferences_dialog)
		display_error_dialog ("Could not play", err->message, GTK_WINDOW (applet->preferences_dialog));
}

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
static void
player_start (AlarmApplet *applet)
{
	if (applet->player == NULL)
		applet->player = media_player_new (get_sound_file (applet),
										   applet->notify_sound_loop,
										   player_state_cb, applet,
										   player_error_cb, applet);
	
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
												   player_error_cb, applet);
	
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

static void
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
		g_debug ("NOTIFICATION TYPE Not yet implemented.");
	}
}

static void
clear_alarm (AlarmApplet *applet)
{
	alarm_gconf_set_started (applet, FALSE);
	applet->alarm_triggered = FALSE;
	
	timer_remove (applet);
	update_label (applet);
	
	// Stop playback if present.
	if (applet->player && applet->player->state == MEDIA_PLAYER_PLAYING)
		media_player_stop (applet->player);
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


// Load stock sounds into list
void
load_stock_sounds_list (AlarmApplet *applet)
{
	AlarmListEntry *entry, *new_entry;
	GList *new, *l;
	guint i, new_stock_len;
	
	//if (applet->sounds != NULL)
	//	alarm_file_entry_list_free (&(applet->sounds));
	
	new = alarm_list_entry_list_new ("file://" ALARM_SOUNDS_DIR,
									 supported_sound_mime_types);
	
	new_stock_len = g_list_length (new);
	
	// Add custom entries if present
	if (g_list_length (applet->sounds) > applet->stock_sounds) {
		l = g_list_nth (applet->sounds, applet->stock_sounds);
		for (; l; l = l->next) {
			entry	  = (AlarmListEntry *) l->data;
			new_entry = alarm_list_entry_new (entry->name, entry->data, entry->icon);
			new = g_list_append (new, new_entry);
		}
	}
	
	alarm_list_entry_list_free(&(applet->sounds));
	
	// Free old stock sounds
	/*for (l = applet->sounds, i = 0; l && i < applet->stock_sounds; l = l->next, i++) {
		alarm_file_entry_free ((AlarmFileEntry *)l->data);
	}
	g_list_free (applet->sounds);*/
	
	// Update stock length and sounds
	applet->sounds = new;
	applet->stock_sounds = new_stock_len;
}

static gchar*
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

// Load stock apps into list
void
load_app_list (AlarmApplet *applet)
{
	AlarmListEntry *entry;
	gchar *filename, *name, *icon, *command;
	xmlDoc *xml_doc;
	xmlNode *root, *section, *element;
    gchar *executable;
	
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
				    	command = gnome_da_xml_get_string (element, "command");
						icon = gnome_da_xml_get_string (element, "icon-name");
						
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

/*
 * }} UI
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
	
	return (const gchar *)e->data;
}

/*
 * UI CALLBACKS {{
 */

static void
menu_set_alarm_cb (BonoboUIComponent *component,
				   gpointer data,
				   const gchar *cname)
{
	AlarmApplet *applet = (AlarmApplet *)data;
	display_set_alarm_dialog (applet);
}

static void
menu_clear_alarm_cb (BonoboUIComponent *component,
					 gpointer data,
					 const gchar *cname)
{
	AlarmApplet *applet = (AlarmApplet *)data;
	
	clear_alarm (applet);
	
	g_debug("alarm_applet_clear_alarm");
}

static void
menu_preferences_cb (BonoboUIComponent *component,
								gpointer data,
								const gchar *cname)
{
	/* Construct the preferences dialog and show it here */
	g_debug("preferences_dialog");
	
	AlarmApplet *applet = (AlarmApplet *)data;
	
	preferences_dialog_display (applet);
}

static void
menu_about_cb (BonoboUIComponent *component,
			   gpointer data,
			   const gchar *cname)
{
	/* Construct the about dialog and show it here */
	g_debug("about_dialog");
}

static gboolean
button_cb (GtkWidget *widget,
						GdkEventButton *event,
						gpointer data)
{
	AlarmApplet *applet = (AlarmApplet *)data;
	
	g_debug("BUTTON: %d", event->button);
	
	/* React only to left mouse button */
	if (event->button == 2 || event->button == 3) {
		return FALSE;
	}
	
	/* TODO: if alarm is triggered { snooze } else */
	
	if (applet->alarm_triggered) {
		g_debug ("Stopping alarm!");
		clear_alarm (applet);
	} else {
		display_set_alarm_dialog (applet);
	}
	
	return TRUE;
}

static void
destroy_cb (GtkObject *object, AlarmApplet *applet)
{
	if (applet->sounds != NULL) {
		alarm_list_entry_list_free(&(applet->sounds));
	}

	timer_remove (applet);
}

/*
 * }} UI CALLBACKS
 */





/*
 * INIT {{
 */

static void
menu_setup (AlarmApplet *applet)
{
	static const gchar *menu_xml =
		"<popup name=\"button3\">\n"
		"   <menuitem name=\"Set Alarm Item\" "
		"			  verb=\"SetAlarm\" "
		"			_label=\"_Set Alarm\" "
		"		   pixtype=\"stock\" "
		"		   pixname=\"gtk-apply\"/>\n"
		"   <menuitem name=\"Clear Item\" "
		"			  verb=\"ClearAlarm\" "
		"			_label=\"_Clear alarm\" "
		"		   pixtype=\"stock\" "
		"		   pixname=\"gtk-clear\"/>\n"
		"   <separator/>\n"
		"   <menuitem name=\"Preferences Item\" "
		"             verb=\"Preferences\" "
		"           _label=\"_Preferences...\"\n"
		"          pixtype=\"stock\" "
		"          pixname=\"gtk-properties\"/>\n"
		"   <menuitem name=\"About Item\" "
		"             verb=\"About\" "
		"           _label=\"_About...\"\n"
		"          pixtype=\"stock\" "
		"          pixname=\"gtk-about\"/>\n"
		"</popup>\n";
	
	static const BonoboUIVerb menu_verbs [] = {
			BONOBO_UI_VERB ("SetAlarm", menu_set_alarm_cb),
			BONOBO_UI_VERB ("ClearAlarm", menu_clear_alarm_cb),
			BONOBO_UI_VERB ("Preferences", menu_preferences_cb),
			BONOBO_UI_VERB ("About", menu_about_cb),
			BONOBO_UI_VERB_END
	};
	
	panel_applet_setup_menu (PANEL_APPLET (applet->parent),
	                         menu_xml,
	                         menu_verbs,
	                         applet);
}

static gboolean
alarm_applet_factory (PanelApplet *panelapplet,
					  const gchar *iid,
					  gpointer data)
{
	AlarmApplet *applet;
	AlarmListEntry *item;
	GtkWidget *hbox;
	GdkPixbuf *icon;
	gchar *tmp;
	
	if (strcmp (iid, "OAFIID:GNOME_AlarmApplet") != 0)
		return FALSE;
	
	
	/* Initialize applet struct */
	applet = g_new(AlarmApplet, 1);
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
	applet->preferences_dialog = NULL;
	
	/* Preferences (defaults) TODO: Add GConf schema */
	applet->show_label = TRUE;
	applet->label_type = LABEL_TYPE_ALARM;
	applet->notify_type = NOTIFY_SOUND;
	applet->notify_sound_loop = TRUE;
	applet->notify_bubble = TRUE;
	
	/* Prepare */
	panel_applet_add_preferences (applet->parent, ALARM_SCHEMA_DIR, NULL);
	panel_applet_set_flags (PANEL_APPLET (panelapplet), PANEL_APPLET_EXPAND_MINOR);
	
	/* Set up gconf handlers */
	setup_gconf (applet);
	
	/* Set up properties menu */
	menu_setup(applet);
	
	/* Set up applet */
	g_signal_connect (G_OBJECT(panelapplet), "button-press-event",
					  G_CALLBACK(button_cb), applet);
	
	g_signal_connect (G_OBJECT(panelapplet), "destroy",
					  G_CALLBACK(destroy_cb), applet);
	
	/* Set up container hbox */
	hbox = gtk_hbox_new(FALSE, 6);
	
	/* Set up icon and label */
	icon = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
									 ALARM_ICON,
									 22,
									 0, NULL);
	
	if (icon == NULL) {
		g_critical ("Icon not found.");
	}
	
	applet->icon = gtk_image_new_from_pixbuf (icon);
	
	if (icon)
		g_object_unref (icon);
	
	applet->label = g_object_new(GTK_TYPE_LABEL,
								 "label", _("No alarm"),
								 "use-markup", TRUE,
								 NULL);
	
	/* Pack */
	gtk_box_pack_start_defaults(GTK_BOX(hbox), applet->icon);
	gtk_box_pack_start_defaults(GTK_BOX(hbox), applet->label);
	
	/* Add to container and show */
	gtk_container_add (GTK_CONTAINER (applet->parent), hbox);
	gtk_widget_show_all (GTK_WIDGET (applet->parent));
	
	
	
	/* Fetch gconf data */
	applet->started       = panel_applet_gconf_get_bool (applet->parent, KEY_STARTED, NULL);
	applet->alarm_time    = panel_applet_gconf_get_int (applet->parent, KEY_ALARMTIME, NULL);
	tmp					  = panel_applet_gconf_get_string (applet->parent, KEY_MESSAGE, NULL);
	
	applet->alarm_message = g_strdup (tmp);
	
	if (applet->started) {
		// Start the timer!
		update_label (applet);
		timer_start (applet);
	}
	
	/* Fetch preference data */
	applet->show_label          = panel_applet_gconf_get_bool (applet->parent, KEY_SHOW_LABEL, NULL);
	applet->notify_sound_loop   = panel_applet_gconf_get_bool (applet->parent, KEY_SOUND_LOOP, NULL);
	applet->notify_command      = panel_applet_gconf_get_string (applet->parent, KEY_COMMAND, NULL);
	applet->notify_bubble       = panel_applet_gconf_get_bool (applet->parent, KEY_NOTIFY_BUBBLE, NULL);
	
	// Load sounds list
	load_stock_sounds_list (applet);
	
	// Fetch sound file
	tmp = panel_applet_gconf_get_string (applet->parent, KEY_SOUND_FILE, NULL);
	
	if (!set_sound_file (applet, tmp)) {
		if (g_list_length (applet->sounds) > 0) {
			// Set it to the first stock sound
			applet->sound_pos = 0;
		}
	}
	g_free(tmp);
	
	tmp = panel_applet_gconf_get_string (applet->parent, KEY_LABEL_TYPE, NULL);
	if (tmp) {
		gconf_string_to_enum (label_type_enum_map, tmp, (gint *)&(applet->label_type));
	}
	g_free(tmp);
	
	tmp = panel_applet_gconf_get_string (applet->parent, KEY_NOTIFY_TYPE, NULL);
	if (tmp) {
		gconf_string_to_enum (notify_type_enum_map, tmp, (gint *)&(applet->notify_type));
	}
	g_free(tmp);
	
	// Set label visibility
	g_object_set (applet->label, "visible", applet->show_label, NULL);
	
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
