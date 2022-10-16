/*
 * alarm-actions.h -- Alarm actions
 *
 * Copyright (C) 2010 Johannes H. Jensen <joh@pseudoberries.com>
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

#include "alarm-actions.h"
#include "alarm-applet.h"
#include "alarm-list-window.h"

#define GET_ACTION(name) GTK_ACTION (gtk_builder_get_object (builder, (name)))

/**
 * Initialize the various actions
 */
void
alarm_applet_actions_init (AlarmApplet *applet)
{
    GtkBuilder *builder = applet->ui;

    // Actions on one alarm
    applet->actions_alarm = gtk_action_group_new ("alarm");

    applet->action_edit = GET_ACTION ("edit-action");
    applet->action_delete = GET_ACTION ("delete-action");
    applet->action_enabled = GTK_TOGGLE_ACTION (GET_ACTION ("enabled-action"));
    applet->action_stop = GET_ACTION ("stop-action");
    applet->action_snooze = GET_ACTION ("snooze-action");

    gtk_action_group_add_action (applet->actions_alarm, applet->action_edit);
    gtk_action_group_add_action (applet->actions_alarm, applet->action_delete);
    gtk_action_group_add_action (applet->actions_alarm, GTK_ACTION (applet->action_enabled));
    gtk_action_group_add_action (applet->actions_alarm, applet->action_stop);
    gtk_action_group_add_action (applet->actions_alarm, applet->action_snooze);



    // Global actions
    applet->actions_global = gtk_action_group_new ("global");

    applet->action_new = GET_ACTION ("new-action");
    applet->action_stop_all = GET_ACTION ("stop-all-action");
    applet->action_snooze_all = GET_ACTION ("snooze-all-action");

    applet->action_toggle_list_win = GTK_TOGGLE_ACTION (GET_ACTION ("toggle-list-win-action"));
    gtk_action_set_accel_group (GTK_ACTION (applet->action_toggle_list_win),
        applet->list_window->accel_group);

    applet->action_toggle_autostart = GTK_TOGGLE_ACTION (GET_ACTION ("autostart-action"));
    applet->action_toggle_show_label = GTK_TOGGLE_ACTION (GET_ACTION ("show-label-action"));

    gtk_action_group_add_action (applet->actions_global, applet->action_new);
    gtk_action_group_add_action (applet->actions_global, applet->action_stop_all);
    gtk_action_group_add_action (applet->actions_global, applet->action_snooze_all);
    gtk_action_group_add_action_with_accel (applet->actions_global,
        GTK_ACTION (applet->action_toggle_list_win), "Escape");
    gtk_action_group_add_action (applet->actions_global, GTK_ACTION (applet->action_toggle_autostart));
    gtk_action_group_add_action (applet->actions_global, GTK_ACTION (applet->action_toggle_show_label));

    gtk_action_connect_accelerator (GTK_ACTION (applet->action_toggle_list_win));

    // Update actions
    alarm_applet_actions_update_sensitive (applet);
}


//
// SINGLE ALARM ACTIONS:
//

/**
 * Edit alarm action
 */
void
alarm_action_edit (GtkAction *action, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
    AlarmListWindow *list_window = applet->list_window;
	Alarm *a = alarm_list_window_get_selected_alarm (list_window);

	if (!a) {
		// No alarms selected
		return;
	}

    g_debug ("AlarmAction: edit '%s'", a->message);

	// Stop alarm
    alarm_clear (a);

    // Show settings dialog for alarm
    alarm_settings_dialog_show (applet->settings_dialog, a);
}

/**
 * Delete alarm action
 */
void
alarm_action_delete (GtkAction *action, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
    AlarmListWindow *list_window = applet->list_window;
    AlarmSettingsDialog *sdialog = applet->settings_dialog;

	Alarm *a = alarm_list_window_get_selected_alarm (list_window);

	if (!a) {
		// No alarms selected
		return;
	}

    g_debug ("AlarmAction: delete '%s'", a->message);

    // If there's a settings dialog open for this alarm, close it.
    if (sdialog->alarm == a) {
        alarm_settings_dialog_close (sdialog);
    }

    // Disable, clear and delete alarm
    alarm_disable (a);
    alarm_clear (a);
    alarm_delete (a);

    // Remove from applet list
    alarm_applet_alarms_remove (applet, a);
}

/**
 * Enable alarm action
 */
void
alarm_action_enabled (GtkToggleAction *action, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
    AlarmListWindow *list_window = applet->list_window;
	Alarm *a = alarm_list_window_get_selected_alarm (list_window);
    gboolean active = gtk_toggle_action_get_active(action);

	if (!a || a->active == active) {
		// No alarms selected or no change
		return;
	}

    g_debug ("AlarmAction: enabled(%d) '%s'", active, a->message);

    alarm_set_enabled (a, active);
}

/**
 * Update enabled action state
 */
void
alarm_action_update_enabled (AlarmApplet *applet)
{
    Alarm *a = alarm_list_window_get_selected_alarm (applet->list_window);
    gboolean active = gtk_toggle_action_get_active(applet->action_enabled);

	if (!a || a->active == active) {
		// No alarms selected or no change
		return;
	}

    gtk_toggle_action_set_active (applet->action_enabled, a->active);
}

/**
 * Stop alarm action
 */
void
alarm_action_stop (GtkAction *action, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
    AlarmListWindow *list_window = applet->list_window;
    Alarm *a;

    if ((a = alarm_list_window_get_selected_alarm (list_window))) {
        g_debug ("AlarmAction: stop '%s'", a->message);

        alarm_clear (a);
    }
}

/**
 * Snooze alarm action
 */
void
alarm_action_snooze (GtkAction *action, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
    AlarmListWindow *list_window = applet->list_window;
    Alarm *a;

    if ((a = alarm_list_window_get_selected_alarm (list_window))) {
        g_debug ("AlarmAction: snooze '%s'", a->message);

        alarm_applet_alarm_snooze (applet, a);
    }
}


//
// GLOBAL ACTIONS
//

/**
 * New alarm action
 */
void
alarm_action_new (GtkAction *action, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
    AlarmListWindow *list_window = applet->list_window;
    Alarm *alarm;
	AlarmListEntry *entry;
    GtkTreeIter iter;
    GtkTreeSelection *selection;

    g_debug ("AlarmAction: new");

	// Create new alarm, will fall back to defaults.
	alarm = alarm_new (ALARM_GCONF_DIR, -1);

	// Set first sound / app in list
	if (applet->sounds != NULL) {
		entry = (AlarmListEntry *)applet->sounds->data;
		g_object_set (alarm, "sound-file", entry->data, NULL);
	}

	if (applet->apps != NULL) {
		entry = (AlarmListEntry *)applet->apps->data;
		g_object_set (alarm, "command", entry->data, NULL);
	}

	// Add alarm to list of alarms
    // This will indirectly add the alarm to the model
	alarm_applet_alarms_add (applet, alarm);

    // Select the new alarm in the list
    if (alarm_list_window_find_alarm (GTK_TREE_MODEL (list_window->model), alarm, &iter)) {
        selection = gtk_tree_view_get_selection (list_window->tree_view);
        gtk_tree_selection_select_iter (selection, &iter);
    }

	// Show edit alarm dialog
    alarm_settings_dialog_show (applet->settings_dialog, alarm);
}

/**
 * Stop all alarms action
 */
void
alarm_action_stop_all (GtkAction *action, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;

    g_debug ("AlarmAction: stop all");

    alarm_applet_alarms_stop (applet);
}

/**
 * Snooze all alarms action
 */
void
alarm_action_snooze_all (GtkAction *action, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;

    g_debug ("AlarmAction: snooze all");

    alarm_applet_alarms_snooze (applet);
}

/**
 * Toggle list window action
 */
void
alarm_action_toggle_list_win (GtkAction *action, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
    AlarmListWindow *list_window = applet->list_window;
    gboolean active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

    g_debug ("AlarmAction: toggle list window");

/*	if (!a || a->active == active) {
		// No alarms selected or no change
		return;
	}
     */

    if (active) {
        alarm_list_window_show (list_window);
    } else {
        alarm_list_window_hide (list_window);
    }
}

/**
 * Quit action
 */
void
alarm_action_quit (GtkAction *action, gpointer data)
{
//    AlarmApplet *applet = (AlarmApplet *)data;

    g_debug ("AlarmAction: Quit!");

    // TODO: Free up resources - maybe use gtk_quit_add() & friends
    gtk_main_quit ();
}

/*
 * Toggle autostart action
 */
void
alarm_action_toggle_autostart (GtkAction *action, gpointer data)
{
	gboolean active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	gboolean autostart_state = prefs_autostart_get_state();
	//gboolean check_active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (applet->pref_autostart_check));

	g_debug ("AlarmAction: toggle autostart to %d", active);

	if (active != autostart_state) {
		g_debug ("AlarmAction: set autostart to %d!", active);
		prefs_autostart_set_state (active);
	}
}

/*
 * Toggle show_label action
 */
void
alarm_action_toggle_show_label (GtkAction *action, gpointer data)
{
	AlarmApplet *applet = (AlarmApplet *)data;
	gboolean active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	gboolean show_label_state = prefs_show_label_get(applet);
	//gboolean check_active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (applet->pref_autostart_check));

	g_debug ("AlarmAction: toggle show_label to %d", active);

	if (active != show_label_state) {
		g_debug ("AlarmAction: set show_label to %d!", active);
		prefs_show_label_set (applet, active);
	}
}








/**
 * Update actions to a consistent state
 */
void
alarm_applet_actions_update_sensitive (AlarmApplet *applet)
{
    //
    // Update single alarm actions:
    //

    // Determine whether there is a selected alarm
    Alarm *a = alarm_list_window_get_selected_alarm (applet->list_window);
    gboolean selected = (a != NULL);

    g_object_set (applet->actions_alarm, "sensitive", selected, NULL);

    g_object_set (applet->action_stop,
        "sensitive", selected && a->triggered, NULL);

    g_object_set (applet->action_snooze,
        "sensitive", selected && a->triggered, NULL);


    //
    // Update global actions
    //

    // If there are alarms triggered, snooze_all and stop_all should be sensitive
    g_object_set (applet->action_stop_all,
        "sensitive", applet->n_triggered > 0, NULL);

    g_object_set (applet->action_snooze_all,
        "sensitive", applet->n_triggered > 0, NULL);

}
