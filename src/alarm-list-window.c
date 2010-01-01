/*
 * alarms-list.d -- Alarm list window
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

#include "alarm-list-window.h"
#include "edit-alarm.h"

void
alarm_list_window_selection_changed (GtkTreeSelection *, gpointer);

static gboolean
alarm_list_window_update_timer (gpointer);


/**
 * Create a new empty Alarm List Window
 */
AlarmListWindow *
alarm_list_window_new (AlarmApplet *applet)
{
	AlarmListWindow *list_window;
    GtkBuilder *builder = applet->ui;
    GtkTreeSelection *selection;
	
    // Initialize struct
	list_window = g_new0 (AlarmListWindow, 1);

	list_window->applet = applet;
	
	list_window->window = GTK_WINDOW (gtk_builder_get_object (builder, "alarm-list-window"));
	list_window->model = GTK_LIST_STORE (gtk_builder_get_object (builder, "alarms-liststore"));
	list_window->tree_view = GTK_TREE_VIEW (gtk_builder_get_object (builder, "alarm-list-view"));

    list_window->new_button = gtk_builder_get_object (builder, "new-button");
    list_window->edit_button = gtk_builder_get_object (builder, "edit-button");
    list_window->delete_button = gtk_builder_get_object (builder, "delete-button");
    list_window->enable_button = gtk_builder_get_object (builder, "enable-button");
    list_window->stop_button = gtk_builder_get_object (builder, "stop-button");
    list_window->snooze_button = gtk_builder_get_object (builder, "snooze-button");

    // Connect some signals
    selection = gtk_tree_view_get_selection (list_window->tree_view);
    g_signal_connect (selection, "changed", 
                      alarm_list_window_selection_changed, applet);

    // Update view every second for pretty countdowns
    g_timeout_add_seconds (1, (GSourceFunc) alarm_list_window_update_timer, applet);

    // Populate with alarms
    alarm_list_window_alarms_add (list_window, applet->alarms);

	return list_window;
}

void
alarm_list_window_show (AlarmListWindow *list_window)
{
	gtk_widget_show_all (GTK_WIDGET (list_window->window));
	gtk_window_present (list_window->window);
}

void
alarm_list_window_hide (AlarmListWindow *list_window)
{
	gtk_widget_hide (GTK_WIDGET (list_window->window));
}

void
alarm_list_window_toggle (AlarmListWindow *list_window)
{
	if (GTK_WIDGET_VISIBLE (list_window->window)) {
		alarm_list_window_hide (list_window);
	} else {
		alarm_list_window_show (list_window);
	}
}


/**
 * Find alarm in the model
 *
 * Returns TRUE if found and sets iter to the location
 * Returns FALSE otherwise
 */
gboolean
alarm_list_window_find_alarm (GtkTreeModel *model,
                              Alarm *alarm,
                              GtkTreeIter *iter)
{
    GtkTreeIter it;
    Alarm *a;
    gboolean valid;

    valid = gtk_tree_model_get_iter_first (model, &it);

    while (valid) {
        gtk_tree_model_get (model, &it, ALARM_COLUMN, &a, -1);
        
        if (a == alarm) {
            if (iter) {
                *iter = it;
            }
            return TRUE;
        }
        
        valid = gtk_tree_model_iter_next(model, &it);
    }

    return FALSE;
}

/**
 * Check whether the list window contains an alarm
 */
gboolean
alarm_list_window_contains (AlarmListWindow *list_window, Alarm *alarm)
{
    return alarm_list_window_find_alarm(list_window->model, alarm, NULL);
}

/**
 * Update the row in the list at the position specified by iter
 */
void
alarm_list_window_update_row (AlarmListWindow *list_window, GtkTreeIter *iter)
{
    GtkTreeModel *model = list_window->model;
    Alarm *a;

    gchar tmp[200];
    gchar *tmp2;
    struct tm *tm;
    
    const gchar *type_col;
    const gchar *time_format;    
    GString *time_col;
    gchar *label_col;

    // Get the alarm at iter
    gtk_tree_model_get (GTK_TREE_MODEL (model), iter,
                        ALARM_COLUMN, &a,
                        -1);
    
    // If alarm is running (active), show remaining time
    if (a->active) {
        tm = alarm_get_remain (a);
    } else {
        tm = alarm_get_time (a);
    }
    
    if (a->type == ALARM_TYPE_CLOCK) {
        type_col = ALARM_ICON;
        time_format = TIME_COL_CLOCK_FORMAT;
    } else {
        type_col = TIMER_ICON;
        time_format = TIME_COL_TIMER_FORMAT;
    }

    // Create time column
    strftime(tmp, sizeof(tmp), time_format, tm);

    time_col = g_string_new (tmp);
    if (a->type == ALARM_TYPE_CLOCK && a->repeat != ALARM_REPEAT_NONE) {
        tmp2 = alarm_repeat_to_pretty (a->repeat);
        g_string_append_printf (time_col, TIME_COL_REPEAT_FORMAT, tmp2);
        g_free (tmp2);
    }

    // Create label column
    label_col = g_strdup_printf (LABEL_COL_FORMAT, a->message);
    
	gtk_list_store_set (model, iter, 
                        TYPE_COLUMN, type_col,
                        TIME_COLUMN, time_col->str,
                        LABEL_COLUMN, label_col,
                        ACTIVE_COLUMN, a->active,
                        -1);

    g_string_free (time_col, TRUE);
    g_free (label_col);
}

/**
 * Add alarm to the list window
 */
void
alarm_list_window_alarm_add (AlarmListWindow *list_window, Alarm *alarm)
{
    GtkListStore *store = list_window->model;
    GtkTreeIter iter;

    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter, ALARM_COLUMN, alarm, -1);

    alarm_list_window_update_row (list_window, &iter);
}

/**
 * Update alarm in the list window
 */
void
alarm_list_window_alarm_update (AlarmListWindow *list_window, Alarm *alarm)
{
    GtkTreeIter iter;

    if (alarm_list_window_find_alarm (list_window->model, alarm, &iter)) {
        alarm_list_window_update_row (list_window, &iter);
    } else {
        g_warning ("alarm_list_window_alarm_update(): Could not find alarm");
    }
}

/**
 * Remove alarm from the list window
 */
void
alarm_list_window_alarm_remove (AlarmListWindow *list_window, Alarm *alarm)
{
    GtkTreeIter iter;

    if (alarm_list_window_find_alarm (list_window->model, alarm, &iter)) {
        gtk_list_store_remove (list_window->model, &iter);
    } else {
        g_warning ("alarm_list_window_alarm_remove(): Could not find alarm");
    }
}

/**
 * Add several alarms to the list window
 */
void
alarm_list_window_alarms_add (AlarmListWindow *list_window, GList *alarms)
{
    AlarmApplet *applet = list_window->applet;
    GtkListStore *model = list_window->model;
    
    GList *l = NULL;
    Alarm *a;

    for (l = applet->alarms; l; l = l->next) {
		a = ALARM (l->data);

        alarm_list_window_alarm_add (list_window, a);
    }
}

/**
 * Update the alarm view every second
 */
static gboolean
alarm_list_window_update_timer (gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
    GtkTreeModel *model = GTK_TREE_MODEL (applet->list_window->model);
    GtkTreeIter iter;
    GtkTreePath *path;
    Alarm *a;
    gboolean valid;

    valid = gtk_tree_model_get_iter_first (model, &iter);

    while (valid) {
        alarm_list_window_update_row (applet->list_window, &iter);
/*        path = gtk_tree_model_get_path (model, &iter);
        gtk_tree_model_row_changed (model, path, &iter);
        gtk_tree_path_free (path);
*/        
        valid = gtk_tree_model_iter_next(model, &iter);
    }
    
    // Keep updating
    return TRUE;
}


//
// Toolbar / tree view callbacks:
//

/**
 * Get the selected alarm
 */
static Alarm *
alarm_list_window_get_selected_alarm (AlarmListWindow *list_window)
{
    GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	Alarm *a;
	
	// Fetch selection
	selection = gtk_tree_view_get_selection (list_window->tree_view);

	if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
		// No alarms selected
		g_debug ("get_selected_alarm: No alarms selected!");
		return NULL;
	}
    
	gtk_tree_model_get (model, &iter, ALARM_COLUMN, &a, -1);
	
	// gtk_tree_model_get () will increase the reference count 
	// of the alarms each time it's called. We dereference it
	// here so they can be properly freed later with g_object_unref()
	g_object_unref (a);
	
	return a;
}

/**
 * Selection changed in tree view
 *
 * Here we update the sensitivity of the toolbar buttons
 */
void
alarm_list_window_selection_changed (GtkTreeSelection *selection, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
    AlarmListWindow *list_window = applet->list_window;

    Alarm *a = alarm_list_window_get_selected_alarm (list_window);

    gboolean sensitive = (a != NULL);

    // Update toolbar
    g_object_set (list_window->edit_button, "sensitive", sensitive, NULL);
    g_object_set (list_window->delete_button, "sensitive", sensitive, NULL);
    g_object_set (list_window->enable_button, "sensitive", sensitive, NULL);
    g_object_set (list_window->stop_button, "sensitive", sensitive, NULL);
    g_object_set (list_window->snooze_button, "sensitive", sensitive, NULL);
}

/**
 * New button clicked
 */
void
alarm_list_window_new_clicked (GtkButton *button, gpointer data)
{
	AlarmApplet *applet = (AlarmApplet *)data;
    AlarmListWindow *list_window = applet->list_window;
    Alarm *alarm;
	AlarmListEntry *entry;
    GtkTreeIter iter;
    GtkTreeSelection *selection;
	
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
    if (alarm_list_window_find_alarm (list_window->model, alarm, &iter)) {
        selection = gtk_tree_view_get_selection (list_window->tree_view);
        gtk_tree_selection_select_iter (selection, &iter);
    }
	
	// Show edit alarm dialog
    alarm_settings_dialog_show (applet->settings_dialog, alarm);
}

/**
 * Edit button clicked
 */
void
alarm_list_window_edit_clicked (GtkButton *button, gpointer data)
{
	AlarmApplet *applet = (AlarmApplet *)data;
    AlarmListWindow *list_window = applet->list_window;
    
	Alarm *a = alarm_list_window_get_selected_alarm (list_window);
	
	if (!a) {
		// No alarms selected
		return;
	}
	
	// Clear any running alarms
	alarm_clear (a);

    //display_edit_alarm_dialog (applet, a);
    alarm_settings_dialog_show (applet->settings_dialog, a);
}

/**
 * Row activated
 */
void
alarm_list_window_row_activated (GtkTreeView       *tree_view,
                                 GtkTreePath       *path,
                                 GtkTreeViewColumn *column,
                                 gpointer           data)
{
    alarm_list_window_edit_clicked (NULL, data);
}

/**
 * Delete button clicked
 */
void
alarm_list_window_delete_clicked (GtkButton *button, gpointer data)
{
	AlarmApplet *applet = (AlarmApplet *)data;
    AlarmListWindow *list_window = applet->list_window;
    AlarmSettingsDialog *sdialog = applet->settings_dialog;
    
	Alarm *a = alarm_list_window_get_selected_alarm (list_window);
    GtkWidget *dialog;
    gint response;
    gchar *title, *text, *secondary_text;

    const gchar *type;
    const gchar *time_format;
    struct tm *tm;
    gchar timestr[50];

    
	if (!a) {
		// No alarms selected
		return;
	}
	
	// Clear any running alarms
	//alarm_clear (a);
    
    // Prompt user
    dialog = gtk_builder_get_object (applet->ui, "delete-dialog");

    // Get time of alarm
    tm = alarm_get_time (a);
    
    if (a->type == ALARM_TYPE_CLOCK) {
        type = _("alarm clock");
        time_format = CLOCK_FORMAT;
    } else {
        type = _("timer");
        time_format = TIMER_FORMAT;
    }

    // Create time string
    strftime(timestr, sizeof(timestr), time_format, tm);

    // Construct the messages
    title = g_strdup_printf (DELETE_DIALOG_TITLE, type);
    text = g_strdup_printf (DELETE_DIALOG_TEXT, type, a->message);
    secondary_text = g_strdup_printf (DELETE_DIALOG_SECONDARY, type, a->message, timestr);
    
    g_object_set (dialog,
                  "title", title,
                  "text", text,
                  "secondary-text", secondary_text,
                  NULL);
    
    g_free (title);
    g_free (text);
    g_free (secondary_text);

    // Run dialog, hide for later use
	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_hide (GTK_WIDGET (dialog));
	
	if (response == GTK_RESPONSE_YES) {
        // DELETE ALARM
        alarm_delete (a);

        // If there's a settings dialog open for this alarm, close it.
        if (sdialog->alarm == a) {
            alarm_settings_dialog_close (sdialog);
        }

        // Remove from applet list
        alarm_applet_alarms_remove (applet, a);

    }
}
