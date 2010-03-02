/*
 * prefs.c -- Alarm Clock global preferences
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

#include <glib.h>
#include <gio/gio.h>

#include "prefs.h"


void
prefs_autostart_init (AlarmApplet *applet);

void
prefs_autostart_update (AlarmApplet *applet);

void
autostart_monitor_changed(GFileMonitor *monitor, GFile *file,
		GFile *other_file, GFileMonitorEvent event_type, gpointer user_data);

/**
 * Initialize preferences dialog and friends
 */
void
prefs_init (AlarmApplet *applet)
{
	applet->prefs_dialog = GTK_DIALOG (gtk_builder_get_object(applet->ui, "preferences"));
	applet->prefs_autostart_check = GTK_WIDGET (gtk_builder_get_object (applet->ui, "autostart-check"));

	prefs_autostart_init (applet);
}

// Ordered list of autostart files we watch for
GFile *autostart_user_file = NULL;
GList *autostart_files = NULL;

/**
 * Initialize autostart preference
 */
void
prefs_autostart_init (AlarmApplet *applet)
{
	// Initialize autostart files
	// We monitor:
	//	~/.config/autostart/PACKAGE.desktop
	//	/etc/xdg/autostart/PACKAGE.desktop
	//	...
	gchar *filename;
	GFile *file;
	GFileMonitor *monitor;
	GList *l = NULL;

	// Add user-specific autostart: ~/.config/autostart/PACKAGE.desktop
	filename = g_build_filename (g_get_user_config_dir (), "autostart", PACKAGE ".desktop", NULL);
	autostart_user_file = g_file_new_for_path (filename);

	autostart_files = g_list_append (autostart_files, autostart_user_file);

	g_free(filename);

	// Add global autostart(s): /etc/xdg/autostart/PACKAGE.desktop
	const gchar* const *sys_cfg_dirs = g_get_system_config_dirs();
	gint i;
	for (i = 0; sys_cfg_dirs[i] != NULL; i++) {
		filename = g_build_filename (sys_cfg_dirs[i], "autostart", PACKAGE ".desktop", NULL);
		file = g_file_new_for_path (filename);

		autostart_files = g_list_append (autostart_files, file);

		g_free(filename);
	}

	//
	// Update autostart state
	//
	prefs_autostart_update (applet);

	//
	// Monitor the autostart files
	//
	for (l = autostart_files; l != NULL; l = l->next) {
		file = G_FILE (l->data);
		monitor = g_file_monitor (file, G_FILE_MONITOR_NONE, NULL, NULL);
		g_signal_connect (monitor, "changed", G_CALLBACK (autostart_monitor_changed), applet);

		filename = g_file_get_path (file);
		g_debug ("Preferences: Autostart watching %s", filename);
		g_free(filename);
	}
}

/**
 * Get the current most important autostart desktop file
 *
 * Returns NULL if no autostart desktop files were found
 */
GFile *
prefs_autostart_get_current ()
{
	GFile *f;
	GList *l = NULL;

	for (l = autostart_files; l != NULL; l = l->next) {
		f = G_FILE (l->data);
		if (g_file_query_exists (f, NULL)) {
			return f;
		}
	}

	return NULL;
}

/**
 * Get the current autostart state
 */
gboolean
prefs_autostart_get_state ()
{
	GFile *file = prefs_autostart_get_current ();
	gchar *filename;
	GKeyFile *kf;
	GError *err = NULL;
	gboolean b;

	g_debug ("Preferences: Autostart get_state for file %p", file);

	if (file) {
		// An autostart file exists, open it and check for Hidden=true and X-GNOME-Autostart-enabled=false
		kf = g_key_file_new ();
		filename = g_file_get_path (file);

		g_debug ("Preferences: Autostart get_state: %s", filename);

		if (g_key_file_load_from_file(kf, filename, G_KEY_FILE_NONE, &err)) {
			g_free (filename);

			// Check for Hidden=true
			b = g_key_file_get_boolean(kf, "Desktop Entry", "Hidden", &err);

			if (!err && b) {
				// Hidden is true, autostart is FALSE
				g_debug ("Preferences: Autostart Hidden=true");
				g_key_file_free (kf);
				return FALSE;
			}

			if (err) {
				g_error_free (err);
				err = NULL;
			}

			// Check for X-GNOME-Autostart-enabled=false
			b = g_key_file_get_boolean(kf, "Desktop Entry", "X-GNOME-Autostart-enabled", &err);

			if (!err && !b) {
				// X-GNOME-Autostart-enabled is false, autostart is FALSE
				g_debug ("Preferences: Autostart X-GNOME-Autostart-enabled=false");
				g_key_file_free (kf);
				return FALSE;
			}

			if (err) {
				g_error_free (err);
				err = NULL;
			}

			g_key_file_free (kf);

		} else {
			g_warning ("Preferences: Could not load autostart-file '%s': %s", filename, err->message);
			g_error_free (err);
			g_free(filename);
		}

		// File exists and none of the keys have been set to disable it => assume autostart is active
		g_debug ("Preferences: Autostart is TRUE");
		return TRUE;
	}

	return FALSE;
}

/**
 * Enable / disable autostart
 */
void
prefs_autostart_set_state (gboolean state)
{
	gboolean current_state = prefs_autostart_get_state();

	if (current_state == state) {
		// No change
		return;
	}

	GFile *file = prefs_autostart_get_current ();
	GFile *f;
	gchar *filename, *str;
	gsize length;
	GKeyFile *kf;
	GFileOutputStream *fstream;
	GError *err = NULL;

	if (state) {
		// Enable
		g_debug ("Preferences: Autostart ENABLE!");
		if (file == autostart_user_file) {
			// Unset Hidden and X-GNOME-Autostart-enabled
			kf = g_key_file_new ();

			filename = g_file_get_path (file);

			if (g_key_file_load_from_file(kf, filename, G_KEY_FILE_NONE, &err)) {
				g_key_file_remove_key (kf, "Desktop Entry", "Hidden", NULL);
				g_key_file_remove_key (kf, "Desktop Entry", "X-GNOME-Autostart-enabled", NULL);

				// Write out results
				fstream = g_file_replace (file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, &err);
				if (fstream) {
					str = g_key_file_to_data (kf, &length, NULL);

					g_print ("Writing str: %s", str);
					g_output_stream_write_all (fstream, str, length, NULL, NULL, &err);
					g_output_stream_close (fstream, NULL, &err);
				}
			}

			if (err) {
				g_warning ("Preferences: Error when enabling autostart-file '%s': %s", filename, err->message);
				g_error_free (err);
			}

			g_free(filename);

		} else {
			// Copy .desktop to autostart_user_file
			filename = g_build_filename (ALARM_CLOCK_DATADIR, "applications", PACKAGE ".desktop", NULL);
			f = g_file_new_for_path (filename);

			if (!g_file_copy (f, autostart_user_file, G_FILE_COPY_NONE, NULL, NULL, NULL, &err)) {
				g_warning ("Preferences: Could not copy '%s' to user config dir: %s", filename, err->message);
				g_error_free (err);
			}

			g_free (filename);
		}

	} else {
		// Disable
		g_debug ("Preferences: Autostart DISABLE!");

		if (file) {

		} else {
			// Disabled already, should not happen
			g_warning ("Preferences: Autostart is already disabled!?!?");
		}
	}

	//
	// Enabling:
	// 1. get_current?
	//	    no: copy .desktop to ~/.config/autostart, finished
	//	    yes: is in user cfg?
	//			no: copy .desktop to ~/.config/autostart, finished
	//			yes: unset Hidden and X-GNOME-Autostart-enabled
	//
	// Disabling:
	// 1. get_current?
	//		no: disabled already, finished
	//		yes: is in user cfg?
	//			no: copy .desktop to ~/.config/autostart, set Hidden=true
	//			yes: set Hidden=true
	//
}

/**
 * Update autostart state
 */
void
prefs_autostart_update (AlarmApplet *applet)
{
	gboolean state = gtk_toggle_action_get_active (applet->action_toggle_autostart);
	gboolean new_state = prefs_autostart_get_state();

	g_debug ("Preferences: Autostart update: new state: %d", new_state);

	if (state != new_state) {
		gtk_toggle_action_set_active (applet->action_toggle_autostart, new_state);
	}
}

/**
 * Show preferences dialog
 */
void
prefs_dialog_show (AlarmApplet *applet)
{
	if (GTK_WIDGET_VISIBLE(applet->prefs_dialog)) {
		gtk_window_present_with_time (GTK_WINDOW (applet->prefs_dialog), gtk_get_current_event_time());
	} else {
		gtk_dialog_run (applet->prefs_dialog);
	}
}

void
autostart_monitor_changed (GFileMonitor     *monitor,
                 GFile            *file,
                 GFile            *other_file,
                 GFileMonitorEvent event_type,
                 gpointer          user_data)
{
	AlarmApplet *applet = (AlarmApplet *)user_data;
	gchar *s = g_file_get_path (file);
	g_print ("Monitor changed on %s: ", s);

	switch (event_type) {
		case G_FILE_MONITOR_EVENT_CHANGED:
			g_print ("CHANGED");
			break;
		case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
			g_print ("CHANGES_DONE_HINT");
			break;
		case G_FILE_MONITOR_EVENT_DELETED:
			g_print ("DELETED");
			break;
		case G_FILE_MONITOR_EVENT_CREATED:
			g_print ("CREATED");
			break;
		case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
			g_print ("ATTRIBUTE_CHANGED");
			break;
		case G_FILE_MONITOR_EVENT_PRE_UNMOUNT:
			g_print ("PRE_UNMOUNT");
			break;
		case G_FILE_MONITOR_EVENT_UNMOUNTED:
			g_print ("UNMOUNTED");
			break;
		default:
			g_print ("UNKNOWN (%d)", event_type);
			break;
	}

	g_print ("\n");

	g_free (s);

	prefs_autostart_update (applet);
}
