/*
 * ui.c - Alarm Clock applet UI routines
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

#include <time.h>
#include <string.h>
#include <libnotify/notify.h>

#include "alarm-applet.h"
#include "alarm-actions.h"
#include "ui.h"

enum
{
    GICON_COL,
    TEXT_COL,
    N_COLUMNS
};

static void alarm_applet_status_init (AlarmApplet *applet);

/*
 * Load a user interface by name
 */
GtkBuilder *
alarm_applet_ui_load (const char *name, AlarmApplet *applet)
{
    GtkBuilder *builder = NULL;
    GError *error = NULL;
    char *filename;
   
    filename = alarm_applet_get_data_path (name);

    g_assert(filename != NULL);
    
    builder = gtk_builder_new();
    
    g_debug ("Loading UI from %s...", filename);

    if (gtk_builder_add_from_file (builder, filename, &error)) {
        /* Connect signals */
        gtk_builder_connect_signals (builder, applet);
    } else {
        g_critical("Couldn't load the interface '%s'. %s", filename, error->message);
        g_error_free (error);
    }

    g_free (filename);

    return builder;
}

void
display_error_dialog (const gchar *message, const gchar *secondary_text, GtkWindow *parent)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (parent,
									GTK_DIALOG_DESTROY_WITH_PARENT,
									GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
									"%s", message);
	
	if (secondary_text != NULL) {
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
												  "%s", secondary_text);
	}
	
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

static gboolean
is_separator (GtkTreeModel *model, GtkTreeIter *iter, gpointer sep_index)
{
	GtkTreePath *path;
	gboolean result;

	path = gtk_tree_model_get_path (model, iter);
	result = gtk_tree_path_get_indices (path)[0] == GPOINTER_TO_INT (sep_index);
	gtk_tree_path_free (path);

	return result;
}

/*
 * Shamelessly stolen from gnome-da-capplet.c
 */
void
fill_combo_box (GtkComboBox *combo_box, GList *list, const gchar *custom_label)
{
	GList *l;
	GtkTreeModel *model;
	GtkCellRenderer *renderer;
	GtkTreeIter iter;
	AlarmListEntry *entry;
	
	g_debug ("fill_combo_box... %d", g_list_length (list));

	gtk_combo_box_set_row_separator_func (combo_box, is_separator,
					  GINT_TO_POINTER (g_list_length (list)), NULL);

	model = GTK_TREE_MODEL (gtk_list_store_new (2, G_TYPE_ICON, G_TYPE_STRING));
	gtk_combo_box_set_model (combo_box, model);
	
	gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo_box));

	renderer = gtk_cell_renderer_pixbuf_new ();

	/* not all cells have a pixbuf, this prevents the combo box to shrink */
	gtk_cell_renderer_set_fixed_size (renderer, -1, 22);
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), renderer, FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), renderer,
					"gicon", GICON_COL,
					NULL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), renderer,
					"text", TEXT_COL,
					NULL);

	for (l = list; l != NULL; l = g_list_next (l)) {
		GIcon *icon;
		
		entry = (AlarmListEntry *) l->data;
		icon = g_icon_new_for_string (entry->icon, NULL);

		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
							GICON_COL, icon,
							TEXT_COL, entry->name,
							-1);

		g_object_unref (icon);
	}

	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, -1);
	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			GICON_COL, NULL,
			TEXT_COL, custom_label,
			-1);
}

/**
 * Show a notification
 */
void
alarm_applet_notification_show (AlarmApplet *applet,
                                const gchar *summary,
                                const gchar *body,
                                const gchar *icon)

{
    NotifyNotification *n;
    GError *error = NULL;

    // Gotta love API breakage...
#ifdef HAVE_LIBNOTIFY_0_7
    n = notify_notification_new (summary, body, icon);
#else
    n = notify_notification_new (summary, body, icon, NULL);
#endif
    
    if (!notify_notification_show (n, &error)) {
        g_warning ("Failed to send notification: %s", error->message);
		g_error_free (error);
	}

	g_object_unref(G_OBJECT(n));
}

void
alarm_applet_label_update (AlarmApplet *applet)
{
#ifdef HAVE_APP_INDICATOR
	GList *l;
	Alarm *a;
	Alarm *next_alarm = NULL;
	struct tm *tm;
	gchar *tmp;
	gboolean show_label = gtk_toggle_action_get_active (applet->action_toggle_show_label);

	if (!show_label) {
		app_indicator_set_label (applet->app_indicator, NULL, NULL);
		return;
	}

	//
	// Show countdown
	//
	for (l = applet->alarms; l; l = l->next) {
		a = ALARM (l->data);
		if (!a->active) continue;

		if (!next_alarm || a->timestamp < next_alarm->timestamp) {
			next_alarm = a;
		}
	}

	if (!next_alarm) {
		// No upcoming alarms
		app_indicator_set_label (applet->app_indicator, NULL, NULL);
		return;
	}

	tm = alarm_get_remain (next_alarm);
	tmp = g_strdup_printf("%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
	app_indicator_set_label (applet->app_indicator, tmp, NULL);
	g_free(tmp);

#endif
}

/*
 * Updates label etc
 */
static gboolean
alarm_applet_ui_update (AlarmApplet *applet)
{
	alarm_applet_label_update (applet);

	//alarm_applet_update_tooltip (applet);

	return TRUE;
}





void
alarm_applet_ui_init (AlarmApplet *applet)
{
    /* Load UI with GtkBuilder */
    applet->ui = alarm_applet_ui_load ("alarm-clock.ui", applet);

    
    /* Initialize status icon */
    alarm_applet_status_init(applet);

    /* Initialize libnotify */
    if (!notify_init (PACKAGE_NAME)) {
        g_critical ("Could not intialize libnotify!");
    }

	/* Initialize alarm list window */
	applet->list_window = alarm_list_window_new (applet);

    /* Initialize alarm settings dialog */
    applet->settings_dialog = alarm_settings_dialog_new (applet);
    
    /* Connect signals */
    //gtk_builder_connect_signals (applet->ui, applet);
    
    /* Initialize actions */
    alarm_applet_actions_init (applet);

    /* Initialize preferences dialog */
    prefs_init (applet);

    /* Set up UI updater */
	alarm_applet_ui_update (applet);
	g_timeout_add_seconds (1, (GSourceFunc)alarm_applet_ui_update, applet);
}

/*
 * Initialize status icon
 */
static void
alarm_applet_status_init (AlarmApplet *applet)
{
	applet->status_menu = GTK_WIDGET (gtk_builder_get_object (applet->ui, "status_menu"));

#ifdef HAVE_APP_INDICATOR
	applet->app_indicator = app_indicator_new(PACKAGE_NAME, ALARM_ICON,
			APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
	app_indicator_set_title (applet->app_indicator, _("Alarm"));
	app_indicator_set_status (applet->app_indicator, APP_INDICATOR_STATUS_ACTIVE);
	app_indicator_set_attention_icon (applet->app_indicator, TRIGGERED_ICON);
	app_indicator_set_menu (applet->app_indicator, GTK_MENU (applet->status_menu));
#else
    applet->status_icon = GTK_STATUS_ICON (gtk_builder_get_object (applet->ui, "status_icon"));

    gtk_status_icon_set_visible (applet->status_icon, TRUE);
#endif
}

/*
 * Update the status icon
 */
void
alarm_applet_status_update (AlarmApplet *applet)
{
#ifdef HAVE_APP_INDICATOR
	if (applet->n_triggered > 0) {
	    app_indicator_set_status (applet->app_indicator, APP_INDICATOR_STATUS_ATTENTION);
	} else {
	    app_indicator_set_status (applet->app_indicator, APP_INDICATOR_STATUS_ACTIVE);
	}
#else
    if (applet->n_triggered > 0) {
        gtk_status_icon_set_from_icon_name (applet->status_icon, TRIGGERED_ICON);
    } else {
        gtk_status_icon_set_from_icon_name (applet->status_icon, ALARM_ICON);
    }
    gtk_status_icon_set_blinking (applet->status_icon, applet->n_triggered > 0);
#endif
}

/*
 * Status icon callbacks:
 */
G_MODULE_EXPORT void
alarm_applet_status_activate (GtkStatusIcon *status_icon,
							  gpointer       user_data)
{
#ifndef HAVE_APP_INDICATOR
    AlarmApplet *applet = (AlarmApplet *)user_data;
    
    // Toggle list window
    gtk_action_activate (GTK_ACTION (applet->action_toggle_list_win));
#endif
}

G_MODULE_EXPORT void
alarm_applet_status_popup (GtkStatusIcon  *status_icon,
                           guint           button,
                           guint           activate_time,
                           gpointer        user_data)
{
#ifndef HAVE_APP_INDICATOR
    AlarmApplet *applet = (AlarmApplet *)user_data;

    gtk_menu_popup (GTK_MENU (applet->status_menu),
                    NULL,
                    NULL,
                    gtk_status_icon_position_menu,
                    status_icon,
                    button,
                    activate_time);
#endif
}

/*
 * Menu callbacks:
 */
/*
G_MODULE_EXPORT void
status_menu_snooze_cb (GtkMenuItem *menuitem,
                       gpointer     user_data)
{
    AlarmApplet *applet = (AlarmApplet *)user_data;
    
	alarm_applet_alarms_snooze (applet);
}

G_MODULE_EXPORT void
status_menu_stop_cb (GtkMenuItem *menuitem,
                      gpointer     user_data)
{
    AlarmApplet *applet = (AlarmApplet *)user_data;
    
	alarm_applet_alarms_stop (applet);
}
*/

void
alarm_applet_status_menu_edit_cb (GtkMenuItem *menuitem,
                     gpointer     user_data)
{
    AlarmApplet *applet = (AlarmApplet *)user_data;

    if (gtk_toggle_action_get_active (applet->action_toggle_list_win)) {
        alarm_list_window_show (applet->list_window);
    } else {
        gtk_toggle_action_set_active (applet->action_toggle_list_win, TRUE);
    }
}       

void
alarm_applet_status_menu_prefs_cb (GtkMenuItem *menuitem,
                      gpointer     user_data)
{
    AlarmApplet *applet = (AlarmApplet *)user_data;
	
	prefs_dialog_show (applet);
}

void
alarm_applet_status_menu_about_cb (GtkMenuItem *menuitem,
                      gpointer     user_data)
{
    AlarmApplet *applet = (AlarmApplet *)user_data;
    gchar *title;
    
    gboolean visible;	
    GtkAboutDialog *dialog = GTK_ABOUT_DIALOG (
        gtk_builder_get_object (applet->ui, "about-dialog"));
    
    g_object_get (dialog, "visible", &visible, NULL);

    if (!visible) {
        // About Alarm Clock
        title = g_strdup_printf (_("About %s"), _(ALARM_NAME));
        g_object_set (G_OBJECT (dialog),
                      "program-name", _(ALARM_NAME),
                      "title", title,
                      "version", VERSION,
                      NULL);
        g_free (title);

        gtk_dialog_run (GTK_DIALOG (dialog));
        
    } else {
        // Already visible, present it
        gtk_window_present (GTK_WINDOW (dialog));
    }
}

/*
 * An error callback for MediaPlayers
 */
void
media_player_error_cb (MediaPlayer *player, GError *err, gpointer data)
{
	GtkWindow *parent = GTK_WINDOW (data);
	gchar *uri, *tmp;
	
	uri = media_player_get_uri (player);
	tmp = g_strdup_printf ("%s: %s", uri, err->message);
	
	g_critical (_("Could not play '%s': %s"), uri, err->message);
	display_error_dialog (_("Could not play"), tmp, parent);
	
	g_free (tmp);
	g_free (uri);
}


/**
 * Alarm changed signal handler
 *
 * Here we update any actions/views, if necessary
 */
void
alarm_applet_alarm_changed (GObject *object,  GParamSpec *pspec, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
    Alarm *alarm = ALARM (object);
    const gchar *pname = pspec->name;
    
    g_debug ("AlarmApplet: Alarm '%s' %s changed", alarm->message, pname);

    // Update Actions
    if (g_strcmp0 (pname, "active") == 0) {
        alarm_action_update_enabled (applet);
    }

    // Update List Window
    if (applet->list_window && GTK_WIDGET_VISIBLE (applet->list_window->window)) {
        // Should really check that the changed param is relevant...
        alarm_list_window_alarm_update (applet->list_window, alarm);
    }

    // Update Settings
    if (applet->settings_dialog && applet->settings_dialog->alarm == alarm) {
        g_debug ("TODO: Update settings dialog");
    }
}

/**
 * Alarm 'alarm' signal handler
 *
 * Here we update any actions/views, if necessary
 */
void
alarm_applet_alarm_triggered (Alarm *alarm, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
    gchar *summary, *body;
    const gchar *icon;

    g_debug ("AlarmApplet: Alarm '%s' triggered", alarm->message);
    
    // Keep track of how many alarms have been triggered
    applet->n_triggered++;

    // Show notification
    summary = g_strdup_printf ("%s", alarm->message);
    body = g_strdup_printf (_("You can snooze or stop alarms from the Alarm Clock menu."));
    icon = (alarm->type == ALARM_TYPE_TIMER) ? TIMER_ICON : ALARM_ICON;
    alarm_applet_notification_show (applet, summary, body, icon);
    
    g_free (summary);
    g_free (body);

    // Update status icon
    alarm_applet_status_update (applet);

    // Update actions
    alarm_applet_actions_update_sensitive (applet);
}

/**
 * Alarm 'cleared' signal handler
 *
 * Here we update any actions/views, if necessary
 */
void
alarm_applet_alarm_cleared (Alarm *alarm, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;

    g_debug ("AlarmApplet: Alarm '%s' cleared", alarm->message);
    
    // Keep track of how many alarms have been triggered
    applet->n_triggered--;
    
    // Update status icon
    alarm_applet_status_update (applet);

    // Update actions
    alarm_applet_actions_update_sensitive (applet);
}
