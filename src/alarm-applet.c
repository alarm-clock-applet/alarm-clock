#include "alarm-applet.h"

#include "alarm.h"
#include "edit-alarm.h"

/*
 * DEFINTIIONS {{
 */

gchar *supported_sound_mime_types [] = { "audio", "video", "application/ogg", NULL };
GHashTable *app_command_map = NULL;

/*
 * }} DEFINTIIONS
 */

/*
 * Clear any running (read: playing sound) alarms.
 */
void
alarm_applet_clear_alarms (AlarmApplet *applet)
{
	GList *l;
	Alarm *a;
	
	g_debug ("Clearing alarms...");
	
	// Loop through alarms and clear all of 'em
	for (l = applet->alarms; l; l = l->next) {
		a = ALARM (l->data);
		alarm_clear (a);
	}
	
	// Close notification if present.
#ifdef HAVE_LIBNOTIFY
	if (applet->notify) {
		alarm_applet_notification_close (applet);
	}
#endif
}

/*
 * TODO: Write me
 * 
 * Should snooze any running snoozable alarms
 */
void
alarm_applet_snooze_alarms (AlarmApplet *applet)
{
	g_debug ("alarm_applet_snooze_alarms: Not yet implemented");
}


/*
 * Sounds list {{
 */

// Load sounds into list
// TODO: Refactor to use a GHashTable with string hash
void
alarm_applet_sounds_load (AlarmApplet *applet)
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
static void
alarm_sound_file_changed (GObject *object, 
						  GParamSpec *param,
						  gpointer data)
{
	Alarm *alarm		= ALARM (object);
	AlarmApplet *applet = (AlarmApplet *)data;
	
	g_debug ("alarm_sound_file_changed: #%d", alarm->id);
	
	// Reload sounds list
	alarm_applet_sounds_load (applet);
}


/*
 * }} Sounds list
 */


/*
 * Apps list {{
 */

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
alarm_applet_apps_load (AlarmApplet *applet)
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


/*
 * Alarm signals {{
 */

/*
 * Find the soonest-to-trigger upcoming alarm
 */
static void
alarm_applet_upcoming_alarm_update (AlarmApplet *applet)
{
	GList *l;
	Alarm *a;
	
	applet->upcoming_alarm = NULL;
	
	/* Loop through alarms looking for all active upcoming alarms and locate
	 * the one which will trigger sooner.
	 */
	
	for (l = applet->alarms; l; l = l->next) {
		a = ALARM (l->data);
		
		if (!a->active) continue;
		
		if (!applet->upcoming_alarm || a->time < applet->upcoming_alarm->time) {
			// This alarm is most recent
			applet->upcoming_alarm = a;
		}
	}
}

/*
 * Callback for when an alarm is activated / deactivated.
 * We use this to update our closest upcoming alarm.
 */
static void
alarm_active_changed (GObject *object, 
					  GParamSpec *param,
					  gpointer data)
{
	Alarm *alarm		= ALARM (object);
	AlarmApplet *applet = (AlarmApplet *)data;
	
	g_debug ("alarm_active_changed: #%d to %d", alarm->id, alarm->active);
	
	// Check if this was the upcoming alarm
	if (!alarm->active) {
		if (applet->upcoming_alarm == alarm) {
			applet->upcoming_alarm = NULL;
			
			alarm_applet_upcoming_alarm_update (applet);
			alarm_applet_label_update (applet);
			
			return;
		}
	}
	
	if (!applet->upcoming_alarm || alarm->time < applet->upcoming_alarm->time) {
		// We're next!
		applet->upcoming_alarm = alarm;
		
		alarm_applet_label_update (applet);
		
		return;
	}
}

/*
 * Callback for when an alarm is triggered
 * We show the notification bubble here if appropriate.
 */
static void
alarm_triggered (Alarm *alarm, gpointer data)
{
	AlarmApplet *applet = (AlarmApplet *)data;
	
	g_debug ("Alarm triggered: #%d", alarm->id);
	
	if (alarm->notify_bubble) {
		g_debug ("Alarm #%d NOTIFICATION DISPLAY", alarm->id);
		alarm_applet_notification_display (applet, alarm);
	}
}

/*
 * }} Alarm signals
 */

/*
 * Alarms list {{
 */

// TODO: Refactor to use a GHashTable instead
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
	
	g_signal_connect (alarm, "notify::sound-file", G_CALLBACK (alarm_sound_file_changed), applet);
	g_signal_connect (alarm, "notify::active", G_CALLBACK (alarm_active_changed), applet);
	g_signal_connect (alarm, "notify::time", G_CALLBACK (alarm_active_changed), applet);
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
	 * If this is the upcoming alarm, update it.
	 */
	if (applet->upcoming_alarm == alarm) {
		alarm_applet_upcoming_alarm_update (applet);
	}
	
	/*
	 * Dereference alarm
	 */
	g_object_unref (alarm);
}

/*
 * }} Alarms list
 */

void
alarm_applet_destroy (AlarmApplet *applet)
{
	GList *l;
	Alarm *a;
	AlarmSettingsDialog *dialog;
	
	g_debug ("AlarmApplet DESTROY");
	
	// Remove any timers
	if (applet->timer_id) {
		g_source_remove (applet->timer_id);
	}
	
	// Destroy alarms list
	if (applet->list_alarms_dialog) {
		list_alarms_dialog_close (applet);
	}
	
	// Destroy preferences dialog
	if (applet->preferences_dialog) {
		gtk_widget_destroy (applet->preferences_dialog);
	}
	
	// Loop through all alarms and free like a mad man!
	for (l = applet->alarms; l; l = l->next) {
		a = ALARM (l->data);
		
		// Check if a dialog is open for this alarm
		dialog = (AlarmSettingsDialog *)g_hash_table_lookup (applet->edit_alarm_dialogs, a->id);
		
		if (dialog) {
			alarm_settings_dialog_close (dialog);
			//g_free (dialog);
		}
		
		g_object_unref (a);
	}
	
	// Remove sounds list
	if (applet->sounds) {
		alarm_list_entry_list_free(&(applet->sounds));
	}
	
	// Remove apps list
	if (applet->apps) {
		alarm_list_entry_list_free(&(applet->apps));
	}
	
	if (app_command_map) {
		g_hash_table_destroy (app_command_map);
		app_command_map = NULL;
	}
	
	// Free GConf dir
	g_free (applet->gconf_dir);
	
	// Finally free the AlarmApplet struct itself
	g_free (applet);
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
	
	if (strcmp (iid, "OAFIID:AlarmClock") != 0)
		return FALSE;
	
	/* Initialize applet struct,
	 * fill with zero's */
	applet = g_new0 (AlarmApplet, 1);
	
	applet->parent = panelapplet;
	applet->edit_alarm_dialogs = g_hash_table_new (NULL, NULL);
	
	/* Preferences (defaults). 
	 * ...gconf_get_string can return NULL if the key is not found. We can't
	 * assume the schema provides the default values for strings. */
	applet->show_label = TRUE;
	applet->label_type = LABEL_TYPE_TIME;
	
	/* Prepare applet */
	panel_applet_add_preferences (applet->parent, ALARM_SCHEMA_DIR, NULL);
	panel_applet_set_flags (PANEL_APPLET (panelapplet), PANEL_APPLET_EXPAND_MINOR);
	applet->gconf_dir = panel_applet_get_preferences_key (applet->parent);
	
	/* Set up gconf handlers */
	alarm_applet_gconf_init (applet);
	
	/* Load gconf values */
	alarm_applet_gconf_load (applet);
	
	/* Load alarms */
	alarm_applet_alarms_load (applet);
	
	/* Find any upcoming active alarms */
	alarm_applet_upcoming_alarm_update (applet);
	
	/* Load sounds from alarms */
	alarm_applet_sounds_load (applet);
	
	/* Load apps for alarms */
	alarm_applet_apps_load (applet);
	
	/* Connect sound_file notify callback to all alarms */
	alarm_signal_connect_list (applet->alarms, "notify::sound-file", 
							   G_CALLBACK (alarm_sound_file_changed), applet);
	
	/* Connect active & time notify callback to all alarms */
	alarm_signal_connect_list (applet->alarms, "notify::active", 
							   G_CALLBACK (alarm_active_changed), applet);
	alarm_signal_connect_list (applet->alarms, "notify::time", 
							   G_CALLBACK (alarm_active_changed), applet);
	
	/* Connect alarm trigger notify to all alarms */
	alarm_signal_connect_list (applet->alarms, "alarm",
							   G_CALLBACK (alarm_triggered), applet);
	
	/* Set up properties menu */
	alarm_applet_menu_init (applet);
	
	/* Set up applet */
	alarm_applet_ui_init (applet);
	
	return TRUE;
}

PANEL_APPLET_BONOBO_FACTORY ("OAFIID:AlarmClock_Factory",
                             PANEL_TYPE_APPLET,
                             "alarm_clock",
                             VERSION,
                             alarm_applet_factory,
                             NULL);


/*
 * }} INIT
 */
