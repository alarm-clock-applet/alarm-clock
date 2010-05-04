/*
 * alarm-applet.c -- Alarm Clock applet bootstrap
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

#include <stdlib.h>

#include "alarm-applet.h"

#include "alarm.h"
#include "alarm-settings.h"

/*
 * DEFINTIIONS {{
 */

const gchar *supported_sound_mime_types [] = { "audio", "video", "application/ogg", NULL };
GHashTable *app_command_map = NULL;

/*
 * }} DEFINTIIONS
 */

/*
 * Snooze any triggered alarms.
 *
 * Returns the number of snoozed alarms
 */
guint
alarm_applet_alarms_snooze (AlarmApplet *applet)
{
	GList *l;
	Alarm *a, *last_snoozed = NULL;
    guint n_snoozed = 0;
    gchar *summary, *body;
    const gchar *icon;

	g_debug ("Snoozing alarms...");

	// Loop through alarms and snooze all triggered ones
	for (l = applet->alarms; l; l = l->next) {
		a = ALARM (l->data);

        if (a->triggered) {
            alarm_applet_alarm_snooze (applet, a);
            last_snoozed = a;
            n_snoozed++;
        }
	}

    // Show notification
    if (n_snoozed > 0) {
        if (n_snoozed == 1) {
            // Single alarm snoozed
            summary = g_strdup_printf (_("%s snoozed"), last_snoozed->message);
            body = g_strdup_printf (_("You can stop the alarm from the Alarm Clock menu."));
            icon = (last_snoozed->type == ALARM_TYPE_TIMER) ? TIMER_ICON : ALARM_ICON;
            alarm_applet_notification_show (applet, summary, body, icon);
        } else {
            // More than 1 alarm snoozed
            summary = g_strdup_printf (_("Snoozed %d alarms"), n_snoozed);
            body = g_strdup_printf (_("You can stop alarms from the Alarm Clock menu."));
            icon = "alarm-clock";
            alarm_applet_notification_show (applet, summary, body, icon);
        }

        g_free (summary);
        g_free (body);
    }
    
    // Reset the triggered counter
    applet->n_triggered = 0;

    // Update status icon
    alarm_applet_status_update (applet);
    
    return n_snoozed;
}

/*
 * Stop any running (read: playing sound) alarms.
 */
guint
alarm_applet_alarms_stop (AlarmApplet *applet)
{
	GList *l;
	Alarm *a, *last_stopped = NULL;
    guint n_stopped = 0;
    gchar *summary, *body;
    const gchar *icon;
    
	g_debug ("Stopping alarms...");

	// Loop through alarms and clear all of 'em
	for (l = applet->alarms; l; l = l->next) {
		a = ALARM (l->data);

        if (alarm_is_playing (a)) {
    		alarm_clear (a);
            last_stopped = a;
            n_stopped++;
        }
	}

    // Show notification
    if (n_stopped > 0) {
        if (n_stopped == 1) {
            // Single alarm stopped
            summary = g_strdup_printf (_("%s stopped"), last_stopped->message);
            body = g_strdup_printf (_("Repeating alarms will still continue according to schedule."));
            icon = (last_stopped->type == ALARM_TYPE_TIMER) ? TIMER_ICON : ALARM_ICON;
            alarm_applet_notification_show (applet, summary, body, icon);
        } else {
            // More than 1 alarm stopped
            summary = g_strdup_printf (_("Stopped %d alarm(s)"), n_stopped);
            body = g_strdup_printf (_("Repeating alarms will still continue according to schedule."));
            icon = "alarm-clock";
            alarm_applet_notification_show (applet, summary, body, icon);
        }

        g_free (summary);
        g_free (body);
    }
    
    // Reset the triggered counter
    applet->n_triggered = 0;

    // Update status icon
    alarm_applet_status_update (applet);
    
    return n_stopped;
}

/**
 * Snooze an alarm, according to UI settings
 */
void
alarm_applet_alarm_snooze (AlarmApplet *applet, Alarm *alarm)
{
    guint mins = applet->snooze_mins;

    if (alarm->type == ALARM_TYPE_CLOCK) {
        // Clocks always snooze for 9 minutes
        mins = ALARM_STD_SNOOZE;
    }

    g_debug ("AlarmApplet: snooze '%s' for %d minutes", alarm->message, mins);

    alarm_snooze (alarm, mins * 60);

    // Update status icon
    alarm_applet_status_update (applet);
}

/**
 * Stop an alarm, keeping UI consistent
 */
void
alarm_applet_alarm_stop (AlarmApplet *applet, Alarm *alarm)
{
    g_debug ("Stopping alarm #%d...", alarm->id);

    // Stop the alarm
    alarm_clear (alarm);

    // Update status icon
    alarm_applet_status_update (applet);
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

	const gchar* const *sysdirs;
	gchar *sounds_dir = NULL;
	gchar *tmp;
	gint i;

	//g_assert (applet->alarms);

	// Free old list
	if (applet->sounds != NULL)
		alarm_list_entry_list_free (&(applet->sounds));

	// Locate gnome sounds
	sysdirs = g_get_system_data_dirs ();
	for (i = 0; !sounds_dir && sysdirs[i] != NULL; i++) {
		tmp = g_build_filename(sysdirs[i], "sounds/gnome/default/alerts", NULL);
		if (g_file_test (tmp, G_FILE_TEST_IS_DIR)) {
			// Load stock sounds
			g_debug ("AlarmApplet: sounds_load: Found %s!", tmp);
			sounds_dir = g_strdup_printf ("file://%s", tmp);
			applet->sounds = alarm_list_entry_list_new (sounds_dir, supported_sound_mime_types);
			g_free (sounds_dir);
		}
		g_free(tmp);
	}

	if (!sounds_dir) {
		g_warning ("AlarmApplet: Could not locate sounds!");
	}

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
				    if (!strcmp (sys_langs[i], (const char *)node_lang)) {
						ret_val = (gchar *) xmlNodeGetContent (element);
						// since sys_langs is sorted from most desirable to
						// least desirable, exit at first match
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

		// `rhythmbox-client --play' doesn't actually start playing unless
		// Rhythmbox is already running. Sounds like a Bug.
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
	const gchar* const *sysdirs;
	gint i;

	if (applet->apps != NULL)
		alarm_list_entry_list_free (&(applet->apps));

	// Locate g-d-a.xml
	sysdirs = g_get_system_data_dirs ();
	for (i = 0; sysdirs[i] != NULL; i++) {
		// We'll get the default media players from g-d-a.xml
		// from gnome-control-center
		filename = g_build_filename (sysdirs[i],
									 "gnome-control-center",
									 "default-apps",
									 "gnome-default-applications.xml",
									 NULL);

		if (g_file_test (filename, G_FILE_TEST_EXISTS)) {
			xml_doc = xmlParseFile (filename);

			if (!xml_doc) {
				g_warning ("Could not load %s.", filename);
				continue;
			}

			root = xmlDocGetRootElement (xml_doc);

			for (section = root->children; section != NULL; section = section->next) {
				if (!xmlStrncmp (section->name, (const xmlChar *)"media-players", 13)) {
					for (element = section->children; element != NULL; element = element->next) {
						if (!xmlStrncmp (element->name, (const xmlChar *)"media-player", 12)) {
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

			g_free(filename);
			break;
		}

		g_free(filename);
	}

//	entry = alarm_list_entry_new("Rhythmbox Music Player", "rhythmbox", "rhythmbox");
//	applet->apps = g_list_append (applet->apps, entry);
}

/*
 * Alarms list {{
 */

// TODO: Refactor to use a GHashTable instead?
void
alarm_applet_alarms_load (AlarmApplet *applet)
{
	GList *list = NULL;
    GList *l = NULL;
    
	if (applet->alarms != NULL) {
		// Free old alarm objects
		for (l = applet->alarms; l != NULL; l = l->next) {
			g_object_unref (ALARM (l->data));
		}

		// Free list
		g_list_free (applet->alarms);
	}

	// Fetch list of alarms and add them
    applet->alarms = NULL;
	list = alarm_get_list (ALARM_GCONF_DIR);

    for (l = list; l != NULL; l = l->next) {
        alarm_applet_alarms_add (applet, ALARM (l->data));
    }
}

void
alarm_applet_alarms_add (AlarmApplet *applet, Alarm *alarm)
{
	applet->alarms = g_list_append (applet->alarms, alarm);

    g_signal_connect (alarm, "notify", G_CALLBACK (alarm_applet_alarm_changed), applet);
	g_signal_connect (alarm, "notify::sound-file", G_CALLBACK (alarm_sound_file_changed), applet);

	g_signal_connect (alarm, "alarm", G_CALLBACK (alarm_applet_alarm_triggered), applet);
	g_signal_connect (alarm, "cleared", G_CALLBACK (alarm_applet_alarm_cleared), applet);

    // Update alarm list window model
    if (applet->list_window) {
        alarm_list_window_alarm_add (applet->list_window, alarm);
    }
}

void
alarm_applet_alarms_remove (AlarmApplet *applet, Alarm *alarm)
{
	// Remove from list
	applet->alarms = g_list_remove (applet->alarms, alarm);

	// Clear list store. This will decrease the refcount of our alarms by 1.
	/*if (applet->list_alarms_store)
		gtk_list_store_clear (applet->list_alarms_store);*/

	g_debug ("alarm_applet_alarms_remove (..., %p): refcount = %d", alarm, G_OBJECT (alarm)->ref_count);
    
	// Remove any signal handlers for this alarm instance.
	g_signal_handlers_disconnect_matched (alarm, 0, 0, 0, NULL, NULL, NULL);

    // Update alarm list window model
    if (applet->list_window) {
        alarm_list_window_alarm_remove (applet->list_window, alarm);
    }
    
	// Dereference alarm
	g_object_unref (alarm);
}

/*
 * }} Alarms list
 */

// TODO: Is this function needed?
/*void
alarm_applet_destroy (AlarmApplet *applet)
{
	GList *l;
	Alarm *a;
	AlarmSettingsDialog *dialog;

	g_debug ("AlarmApplet DESTROY");

	// TODO: Destroy alarms list
//	if (applet->list_alarms_dialog) {
//		list_alarms_dialog_close (applet);
//	}

	// Destroy preferences dialog
	if (applet->prefs_dialog) {
		gtk_widget_destroy (GTK_WIDGET (applet->prefs_dialog));
	}

	// Loop through all alarms and free like a mad man!
	for (l = applet->alarms; l; l = l->next) {
		a = ALARM (l->data);

		// Check if a dialog is open for this alarm
		//dialog = (AlarmSettingsDialog *)g_hash_table_lookup (applet->edit_alarm_dialogs, (gconstpointer)a->id);

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
	//g_free (applet->gconf_dir);

	// Finally free the AlarmApplet struct itself
	g_free (applet);
}*/


static UniqueResponse
unique_app_message_cb (UniqueApp         *app,
                       UniqueCommand      command,
                       UniqueMessageData *message,
                       guint              time_,
                       gpointer           user_data)
{
    AlarmApplet *applet = (AlarmApplet *)user_data;

    UniqueResponse res;

    switch (command) {
        case UNIQUE_ACTIVATE:
            g_debug ("AlarmApplet: unique_app_message: ACTIVATE");
            if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (applet->action_toggle_list_win))) {
                // Already visible, present to user
                alarm_list_window_show (applet->list_window);
            } else {
                // Toggle list window visibility
                gtk_action_activate (GTK_ACTION (applet->action_toggle_list_win));
            }
            
            res = UNIQUE_RESPONSE_OK;
            break;
        default:
            g_warning ("AlarmApplet: unique_app_message: Unknown command %d",
                command);
            
            res = UNIQUE_RESPONSE_INVALID;
            break;
    }

    return res;
}



/*
 * INIT {{
 */

static AlarmApplet*
alarm_applet_init (int *argc, char ***argv)
{
	AlarmApplet *applet;
	UniqueApp *unique_app;

	GError *error = NULL;
	GOptionContext *context;

	gboolean hidden = FALSE;     // Start hidden

    // Command line options
	GOptionEntry entries[] =
	{
		{ "hidden", 0, 0, G_OPTION_ARG_NONE, &hidden, "Start hidden", NULL },
		{ NULL }
	};

	// Initialize unique app
	unique_app = unique_app_new ("com.pseudoberries.AlarmClock", NULL);

	// Check if we're already running
	if (unique_app_is_running (unique_app)) {
		g_printerr(_("%s is already running, exiting...\n"), PACKAGE);

		// Send activate message
		UniqueMessageData *message = unique_message_data_new ();

		unique_app_send_message (unique_app, UNIQUE_ACTIVATE, message);

		unique_message_data_free (message);
		g_object_unref (unique_app);

		exit (EXIT_SUCCESS);
	}
	
	// Parse command-line arguments
	context = g_option_context_new (NULL);
	g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
	g_option_context_add_group (context, gtk_get_option_group (TRUE));

	if (!g_option_context_parse (context, argc, argv, &error)) {
		g_printerr ("%s\n", error->message);
		exit (EXIT_FAILURE);
	}

	// Initialize applet struct
	applet = g_new0 (AlarmApplet, 1);

	// Set up unique app
	applet->unique_app = unique_app;

	g_signal_connect (unique_app, "message-received",
		G_CALLBACK (unique_app_message_cb), applet);
    
	//applet->edit_alarm_dialogs = g_hash_table_new (NULL, NULL);

	// Preferences (defaults).
	// ...gconf_get_string can return NULL if the key is not found. We can't
	// assume the schema provides the default values for strings.

	// TODO: Add to gconf
	applet->snooze_mins = 5;

	// Set up gconf handlers
	alarm_applet_gconf_init (applet);

	// Load gconf values
	alarm_applet_gconf_load (applet);

	// Load alarms
	alarm_applet_alarms_load (applet);
    
	// Load sounds from alarms
	alarm_applet_sounds_load (applet);

	// Load apps for alarms
	alarm_applet_apps_load (applet);

	// Set up applet UI
	alarm_applet_ui_init (applet);

	// Show alarms window, unless --hidden
	if (!hidden) {
		gtk_action_activate (GTK_ACTION (applet->action_toggle_list_win));
	}

	return applet;
}

/**
 * Cleanup
 */
static void
alarm_applet_quit (AlarmApplet *applet)
{
    g_debug ("AlarmApplet: Quitting...");

    g_object_unref (applet->unique_app);
}

/**
 * Alarm Clock main()
 */
int
main (int argc, char *argv[])
{
	AlarmApplet *applet;

    // Internationalization
    bindtextdomain (GETTEXT_PACKAGE, ALARM_CLOCK_DATADIR "/locale");
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    // Terminate on critical errors
    //g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL);

	// Initialize GTK+
	gtk_init (&argc, &argv);

	// Initialize applet
	applet = alarm_applet_init (&argc, &argv);
    
	// Start main loop
	gtk_main ();
	
	// Clean up
	alarm_applet_quit (applet);

	return 0;
}

/*
 * }} INIT
 */
