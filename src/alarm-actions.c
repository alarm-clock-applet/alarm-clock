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

#define GET_ACTION(map, name) G_SIMPLE_ACTION(g_action_map_lookup_action(G_ACTION_MAP(map), (name)))

/**
 * Initialize the various actions
 */
void
alarm_applet_actions_init (AlarmApplet *applet)
{
    GtkBuilder *builder = applet->ui;

    // Actions on one alarm
    const GActionEntry win_action_entries[] = {
        { "alarm_new", alarm_action_new, },
        { "alarm_edit", alarm_action_edit, },
        { "alarm_delete", alarm_action_delete, },
        { "alarm_stop", alarm_action_stop, },
        { "alarm_snooze", alarm_action_snooze, },
        { "alarm_enable", alarm_action_enable, NULL, "false"},
    };
    g_action_map_add_action_entries(G_ACTION_MAP(applet->list_window->window), win_action_entries, G_N_ELEMENTS(win_action_entries), applet);

    applet->action_edit = GET_ACTION(applet->list_window->window, "alarm_edit");
    applet->action_delete = GET_ACTION(applet->list_window->window, "alarm_delete");
    applet->action_enable = GET_ACTION(applet->list_window->window, "alarm_enable");
    applet->action_stop = GET_ACTION(applet->list_window->window, "alarm_stop");
    applet->action_snooze = GET_ACTION(applet->list_window->window, "alarm_snooze");

    // Global actions
    const GActionEntry app_action_entries[] = {
        { "snooze_all", alarm_action_snooze_all, },
        { "stop_all", alarm_action_stop_all, },
        { "quit", alarm_action_quit, },
        { "show_alarms_list", alarm_action_toggle_list_win, },
        { "autostart", alarm_action_toggle_autostart, NULL, "false"},
        { "show_countdown", alarm_action_toggle_show_label, NULL, "false"},
    };
    g_action_map_add_action_entries(G_ACTION_MAP(applet->application), app_action_entries, G_N_ELEMENTS(app_action_entries), applet);

    applet->action_toggle_list_win = GET_ACTION(applet->application, "show_alarms_list");
    applet->action_toggle_autostart = GET_ACTION(applet->application, "autostart");
    applet->action_toggle_show_label = GET_ACTION(applet->application, "show_countdown");
    g_simple_action_set_enabled(applet->action_toggle_show_label, TRUE);

    // FIXME: Add more such as Ctrl Q, Ctrl N, etc etc
    const gchar* const toggle_list_win_accel[] = {
        "Escape",
        NULL
    };
    gtk_application_set_accels_for_action(applet->application, "app.show_alarms_list", toggle_list_win_accel);

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
alarm_action_edit (GSimpleAction *action, GVariant *parameter, gpointer data)
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
alarm_action_delete (GSimpleAction *action, GVariant *parameter, gpointer data)
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
alarm_action_enable (GSimpleAction *action, GVariant *parameter, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
    AlarmListWindow *list_window = applet->list_window;
	Alarm *a = alarm_list_window_get_selected_alarm (list_window);
    GVariant* state = g_action_get_state(G_ACTION(action));
    if(!state)
        return;

    gboolean active = !g_variant_get_boolean(state);
    g_variant_unref(state);

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
    GVariant* state = g_action_get_state(G_ACTION(applet->action_enable));
    if(!state)
        return;

    gboolean active = g_variant_get_boolean(state);
    g_variant_unref(state);

    if (!a || a->active == active) {
        // No alarms selected or no change
        return;
    }

    state = g_variant_new("b", a->active);
    g_simple_action_set_state(applet->action_enable, state);
}

/**
 * Stop alarm action
 */
void
alarm_action_stop (GSimpleAction *action, GVariant *parameter, gpointer data)
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
alarm_action_snooze (GSimpleAction *action, GVariant *parameter, gpointer data)
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
alarm_action_new (GSimpleAction *action, GVariant *parameter, gpointer data)
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
alarm_action_stop_all (GSimpleAction *action, GVariant *parameter, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;

    g_debug ("AlarmAction: stop all");

    alarm_applet_alarms_stop (applet);
}

/**
 * Snooze all alarms action
 */
void
alarm_action_snooze_all (GSimpleAction *action, GVariant *parameter, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;

    g_debug ("AlarmAction: snooze all");

    alarm_applet_alarms_snooze (applet);
}

/**
 * Toggle list window action
 */
void
alarm_action_toggle_list_win (GSimpleAction *action, GVariant *parameter, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
    AlarmListWindow *list_window = applet->list_window;
    gboolean active = !gtk_widget_get_mapped(GTK_WIDGET(list_window->window));

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
alarm_action_quit (GSimpleAction *action, GVariant *parameter, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;

    g_debug ("AlarmAction: Quit!");

    // TODO: Free up resources
    g_application_quit(G_APPLICATION(applet->application));
}

/*
 * Toggle autostart action
 */
void
alarm_action_toggle_autostart (GSimpleAction *action, GVariant *parameter, gpointer data)
{
    GVariant* state = g_action_get_state(G_ACTION(action));
    if(!state)
        return;

    gboolean active = !g_variant_get_boolean(state);
    g_variant_unref(state);

	gboolean autostart_state = prefs_autostart_get_state();

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
alarm_action_toggle_show_label (GSimpleAction *action, GVariant *parameter, gpointer data)
{
	AlarmApplet *applet = (AlarmApplet *)data;
    GVariant* state = g_action_get_state(G_ACTION(action));
    if(!state)
        return;

    gboolean active = !g_variant_get_boolean(state);
    g_variant_unref(state);
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

    g_simple_action_set_enabled(applet->action_enable, selected);
    g_simple_action_set_enabled(applet->action_edit, selected);
    g_simple_action_set_enabled(applet->action_delete, selected);
    g_simple_action_set_enabled(applet->action_stop, selected && a->triggered);
    g_simple_action_set_enabled(applet->action_snooze, selected && a->triggered);


    //
    // Update global actions
    //

    // If there are alarms triggered, snooze_all and stop_all should be sensitive
    g_simple_action_set_enabled(GET_ACTION(applet->application, "stop_all"), applet->n_triggered > 0);
    g_simple_action_set_enabled(GET_ACTION(applet->application, "snooze_all"), applet->n_triggered > 0);

    // Perhaps not the best place for this (as it's not an action)
    // This is needed because the action being disabled does not affect the GtkMenuButton
    gtk_widget_set_sensitive(applet->list_window->snooze_button, selected && a->triggered);
}
