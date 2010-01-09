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
#include "alarm-settings.h"

void
alarm_list_window_selection_changed (GtkTreeSelection *, gpointer);

static gboolean
alarm_list_window_update_timer (gpointer);

static void
alarm_list_window_alarm_changed (GObject *object, GParamSpec *param, gpointer data);

static void
alarm_list_window_alarm_triggered (Alarm *alarm, gpointer data);

static gint
alarm_list_window_sort_iter_compare (GtkTreeModel *model, 
                                     GtkTreeIter *a, GtkTreeIter *b,
                                     gpointer data);
/*
static void
alarm_list_window_init_snooze_menu (AlarmListWindow *list_window);
*/
void
alarm_list_window_snooze_menu_activate (GtkMenuItem *item, gpointer data);

/**
 * Create a new empty Alarm List Window
 */
AlarmListWindow *
alarm_list_window_new (AlarmApplet *applet)
{
	AlarmListWindow *list_window;
    GtkBuilder *builder = applet->ui;
    GtkTreeSelection *selection;
    GtkTreeSortable *sortable;
	
    // Initialize struct
	list_window = g_new0 (AlarmListWindow, 1);

	list_window->applet = applet;

    // Widgets
	list_window->window = GTK_WINDOW (gtk_builder_get_object (builder, "alarm-list-window"));
	list_window->model = GTK_LIST_STORE (gtk_builder_get_object (builder, "alarms-liststore"));
	list_window->tree_view = GTK_TREE_VIEW (gtk_builder_get_object (builder, "alarm-list-view"));

    list_window->new_button = gtk_builder_get_object (builder, "new-button");
    list_window->edit_button = gtk_builder_get_object (builder, "edit-button");
    list_window->delete_button = gtk_builder_get_object (builder, "delete-button");
    list_window->enable_button = gtk_builder_get_object (builder, "enable-button");
    list_window->stop_button = gtk_builder_get_object (builder, "stop-button");
    list_window->snooze_button = gtk_builder_get_object (builder, "snooze-button");
    list_window->snooze_menu = gtk_builder_get_object (builder, "snooze-menu");

    // Actions
    list_window->snooze_action = gtk_builder_get_object (builder, "snooze-action");
    
    // Connect some signals
    selection = gtk_tree_view_get_selection (list_window->tree_view);
    g_signal_connect (selection, "changed", 
                      alarm_list_window_selection_changed, applet);

    // Update view every second for pretty countdowns
    g_timeout_add_seconds (1, (GSourceFunc) alarm_list_window_update_timer, applet);

    // Set up sorting
    sortable = GTK_TREE_SORTABLE (list_window->model);
    
    gtk_tree_sortable_set_sort_func (sortable, SORTID_TIME_REMAINING,
        alarm_list_window_sort_iter_compare, GINT_TO_POINTER (SORTID_TIME_REMAINING),
        NULL);

    // Set initial sort order
    gtk_tree_sortable_set_sort_column_id (sortable, SORTID_TIME_REMAINING, GTK_SORT_ASCENDING);

    // Populate with alarms
    alarm_list_window_alarms_add (list_window, applet->alarms);
    
    // Update snooze menu
    alarm_list_window_snooze_menu_update (list_window);

	return list_window;
}

/*
static void
alarm_list_window_init_snooze_menu (AlarmListWindow *list_window)
{
    GtkMenu *snooze_menu = GTK_MENU (list_window->snooze_menu);
    GSList *group = NULL;
    GtkWidget *item;
    
    guint items[] = {1, 3, 5, 10, NULL};
    guint mins;
    gchar *label;
    gint i;

    for (i = 0; items[i]; i++) {
        g_debug ("i is %d", i);
        mins = items[i];
        if (mins == 1) {
            label = g_strdup (_("1 minute"));
        } else {
            label = g_strdup_printf (_("%d minutes"), mins);
        }

        item = gtk_radio_menu_item_new_with_label (group, label);
        group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
        
        g_object_set_data (item, "snooze", GUINT_TO_POINTER (mins));

        if (mins == list_window->snooze_mins) {
            gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
        }

        g_signal_connect (item, "activate",
            alarm_list_window_snooze_menu_activate, list_window->applet);


        g_debug ("Adding snooze %d...", mins);
        
        gtk_menu_append (snooze_menu, item);

        g_free (label);
    }

    item = gtk_menu_item_new_with_label (_("Custom..."));
    gtk_menu_append (snooze_menu, item);

    gtk_widget_show_all (snooze_menu);
}
*/

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
        gtk_tree_model_get (model, &it, COLUMN_ALARM, &a, -1);
        
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
                        COLUMN_ALARM, &a,
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
                        COLUMN_TYPE, type_col,
                        COLUMN_TIME, time_col->str,
                        COLUMN_LABEL, label_col,
                        COLUMN_ACTIVE, a->active,
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
    gtk_list_store_set (store, &iter, COLUMN_ALARM, alarm, -1);

    // Notify of changes
    g_signal_connect (alarm, "notify",
        G_CALLBACK (alarm_list_window_alarm_changed), list_window);

    g_signal_connect (alarm, "alarm",
        G_CALLBACK (alarm_list_window_alarm_triggered), list_window);

    g_signal_connect (alarm, "cleared",
        G_CALLBACK (alarm_list_window_alarm_triggered), list_window);

    alarm_list_window_update_row (list_window, &iter);
}

/**
 * Update alarm in the list window
 */
void
alarm_list_window_alarm_update (AlarmListWindow *list_window, Alarm *alarm)
{
    GtkTreeIter iter;

    g_debug ("AlarmListWindow alarm_update: %p (%s)", alarm, alarm->message);

    if (alarm_list_window_find_alarm (list_window->model, alarm, &iter)) {
        alarm_list_window_update_row (list_window, &iter);
        alarm_list_window_update_toolbar (list_window);
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

        // Disconnect notify handler
        g_signal_handlers_disconnect_matched (alarm, G_SIGNAL_MATCH_FUNC, 
            0, 0, NULL, alarm_list_window_alarm_changed, NULL);
        
    } else {
        g_warning ("alarm_list_window_alarm_remove(): Could not find alarm");
    }
}

/**
 * Callback for when an alarm changes
 */
static void
alarm_list_window_alarm_changed (GObject *object, 
                                 GParamSpec *param,
                                 gpointer data)
{
	Alarm *alarm = ALARM (object);
    AlarmListWindow *list_window = (AlarmListWindow *)data;

    // Update alarm in window
    alarm_list_window_alarm_update (list_window, alarm);
}

/**
 * Callback for when an alarm is triggered / cleared
 */
static void
alarm_list_window_alarm_triggered (Alarm *alarm, gpointer data)
{
	AlarmListWindow *list_window = (AlarmApplet *)data;

    // Update alarm in window
    alarm_list_window_alarm_update (list_window, alarm);
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
        valid = gtk_tree_model_iter_next(model, &iter);
    }
    
    // Keep updating
    return TRUE;
}

/**
 * Sort compare function
 */
static gint
alarm_list_window_sort_iter_compare (GtkTreeModel *model, 
                                     GtkTreeIter *i1, GtkTreeIter *i2,
                                     gpointer data)
{
    gint sortcol = GPOINTER_TO_INT (data);
    Alarm *a1, *a2;
    gint ret = 0;

    // Fetch ze alarms
    gtk_tree_model_get (model, i1, COLUMN_ALARM, &a1, -1);
    gtk_tree_model_get (model, i2, COLUMN_ALARM, &a2, -1);

    switch (sortcol) {
        case SORTID_TIME_REMAINING:
        {
            // Sort by remaining time
            time_t t1, t2;

            // Show active alarms first
            if (a1->active && a2->active) {
                t1 = alarm_get_remain_seconds (a1);
                t2 = alarm_get_remain_seconds (a2);
                
            } else if (a1->active && !a2->active) {
                t1 = 0;
                t2 = 1;
            } else if (!a1->active && a2->active) {
                t1 = 1;
                t2 = 0;
            } else {
                // Both inactive, sort by time
                t1 = a1->time;
                t2 = a2->time;
            }

            ret = t1 - t2;
        }
            break;
        default:
            g_return_val_if_reached (0);
    }

    return ret;
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
		//g_debug ("get_selected_alarm: No alarms selected!");
		return NULL;
	}
    
	gtk_tree_model_get (model, &iter, COLUMN_ALARM, &a, -1);
	
	// gtk_tree_model_get () will increase the reference count 
	// of the alarms each time it's called. We dereference it
	// here so they can be properly freed later with g_object_unref()
    // Ugh, we use gtk_tree_model_get a lot, is there no other way?
	//g_object_unref (a);
	
	return a;
}

/**
 * Set the selected alarm
 */
static void
alarm_list_window_select_alarm (AlarmListWindow *list_window, Alarm *alarm)
{
    GtkTreeModel *model = GTK_TREE_MODEL (list_window->model);
    GtkTreeSelection *selection;
    GtkTreeIter iter;

    if (!alarm_list_window_find_alarm (model, alarm, &iter)) {
        g_warning ("AlarmListWindow select_alarm: Alarm %p not found!", alarm);
        return;
    }
    
    selection = gtk_tree_view_get_selection (list_window->tree_view);

    gtk_tree_selection_select_iter (selection, &iter);
}

/**
 * Update toolbar to a consistent state
 */
void
alarm_list_window_update_toolbar (AlarmListWindow *list_window)
{
    Alarm *a = alarm_list_window_get_selected_alarm (list_window);

    gboolean sensitive = (a != NULL);

    g_debug ("Update toolbar");

    // Update toolbar
    g_object_set (list_window->edit_button, "sensitive", sensitive, NULL);
    g_object_set (list_window->delete_button, "sensitive", sensitive, NULL);
    
    g_object_set (list_window->enable_button, 
        "sensitive", sensitive && !a->active, NULL);
    
    g_object_set (list_window->stop_button, 
        "sensitive", sensitive && (a->active || a->triggered), NULL);
    
    g_object_set (list_window->snooze_action, 
        "sensitive", sensitive && a->triggered, NULL);
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
    Alarm *a = list_window->selected_alarm;

    //g_debug ("AlarmListWindow selection-changed reordered=%d toggled=%d", 
    //    list_window->reordered, list_window->toggled);

    // If rows have just been reordered, retain the selected row
    // But only if the reordering was triggered by a click on the toggle cell
    if (list_window->reordered && list_window->toggled) {
        //g_debug ("AlarmListWindow: selection-changed REORDERED & TOGGLED!");
        
        list_window->reordered = FALSE;
        list_window->toggled = FALSE;

//        g_debug ("AlarmListWindow: selection-changed selecting %p", list_window->selected_alarm);
        alarm_list_window_select_alarm (list_window, list_window->selected_alarm);
        
        return;
    }

    // Reset reordered and toggled flags
    list_window->reordered = FALSE;
    list_window->toggled = FALSE;
    
    // Update toolbar
    alarm_list_window_update_toolbar (list_window);
    
    // Update selected alarm (might be NULL)
    list_window->selected_alarm = alarm_list_window_get_selected_alarm (list_window);

    
    g_debug ("AlarmListWindow: selection-changed from %s (%p) to %s (%p)",
        (a) ? a->message : "<none>", a,
        (list_window->selected_alarm) ? list_window->selected_alarm->message : "<none>",
        list_window->selected_alarm);
}

/**
 * Toggle cell changed
 */
void
alarm_list_window_enable_toggled (GtkCellRendererToggle *cell_renderer,
                                  gchar *path,
                                  gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
    AlarmListWindow *list_window = applet->list_window;
    GtkTreeModel *model = GTK_TREE_MODEL (list_window->model);
    GtkTreeIter iter;
    Alarm *a;
    
    if (gtk_tree_model_get_iter_from_string (model, &iter, path)) {
        gtk_tree_model_get (model, &iter, COLUMN_ALARM, &a, -1);

        g_debug ("AlarmListWindow enable toggled on %p", a);
        
        // Reset reordered flag
        list_window->reordered = FALSE;
        
        // Select the toggled alarm
        alarm_list_window_select_alarm (list_window, a);

        // Let selection_changed know an alarm was just toggled so
        // this alarm is re-selected if the rows are reordered
        list_window->toggled = TRUE;
        
        // Toggle enabled state
        alarm_set_enabled (a, !a->active);
    }
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
	
	// Stop alarm
    alarm_clear (a);
    
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
 * Rows reordered
 */
void
alarm_list_window_rows_reordered (GtkTreeModel *model,
                                  GtkTreePath  *path,
                                  GtkTreeIter  *iter,
                                  gpointer      arg3,
                                  gpointer      data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
    AlarmListWindow *list_window = applet->list_window;
    Alarm *a;

    // Return if list_window is not set. This happens during initialization.
    if (!list_window)
        return;
    
    // Retain selected alarm
    a = alarm_list_window_get_selected_alarm (list_window);
    
    // Signal to selection_changed that the rows have been reordered.
    list_window->reordered = TRUE;
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

/**
 * Enable button clicked
 */
void
alarm_list_window_enable_clicked (GtkButton *button, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
    AlarmListWindow *list_window = applet->list_window;
    Alarm *a;
    
    if (a = alarm_list_window_get_selected_alarm (list_window)) {
        alarm_enable (a);
    }
}


/**
 * Stop button clicked
 */
void
alarm_list_window_stop_clicked (GtkButton *button, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
    AlarmListWindow *list_window = applet->list_window;
    Alarm *a;
    
    if (a = alarm_list_window_get_selected_alarm (list_window)) {
        if (a->triggered) {
            alarm_clear (a);
        } else {
            alarm_disable (a);
        }
    }
}

/**
 * Snooze button clicked
 */
/*void
alarm_list_window_snooze_clicked (GtkButton *button, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
    AlarmListWindow *list_window = applet->list_window;
    Alarm *a;
    
    if (a = alarm_list_window_get_selected_alarm (list_window)) {
        alarm_snooze (a, 60);
    }
}*/

void
alarm_snooze_action (GtkAction *action, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
    AlarmListWindow *list_window = applet->list_window;
    Alarm *a;
    
    if (a = alarm_list_window_get_selected_alarm (list_window)) {
        g_debug ("AlarmListWindow: Snooze Action: %s for %d mins", a->message, applet->snooze_mins);
        alarm_snooze (a, applet->snooze_mins * 60);
    }
}

void
alarm_list_window_snooze_menu_activated (GtkMenuItem *menuitem,
                                         gpointer          data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
    AlarmListWindow *list_window = applet->list_window;
    gchar **parts;
    guint i;
    guint mins;

//    g_debug ("AlarmListWindow: snooze-menu activated %s to %d", 
//        gtk_menu_item_get_label (menuitem), gtk_check_menu_item_get_active (menuitem));

    if (gtk_check_menu_item_get_active (menuitem)) {
        // Determine #mins from the name of the menu item (hackish)
        // Assumes name follows the format {foo}-{mins}
        parts = g_strsplit (gtk_widget_get_name (GTK_WIDGET (menuitem)), "-", 0);
        for (i = 0; parts[i] != NULL; i++)
            // Loop to the last element
            ;
        
        mins = g_strtod (parts[i-1], NULL);
        
        g_debug ("AlarmListWindow: snooze-menu activated: Snooze for %d mins!", mins);

        applet->snooze_mins = mins;

        gtk_action_activate (list_window->snooze_action);
    }
}

void
alarm_list_window_snooze_menu_custom_activated (GtkMenuItem *menuitem,
                                                gpointer          data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
    AlarmListWindow *list_window = applet->list_window;
    GtkWidget *dialog, *spin;
    gint response;
    Alarm *a;
    guint mins;

    g_debug ("AlarmListWindow: snooze-menu custom activated");
    
    dialog = gtk_builder_get_object (applet->ui, "snooze-dialog");
    spin = gtk_builder_get_object (applet->ui, "snooze-spin");

    // Run dialog, hide for later use
	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_hide (GTK_WIDGET (dialog));

	if (response == GTK_RESPONSE_OK) {
        mins = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin));
        if (a = alarm_list_window_get_selected_alarm (list_window)) {
            g_debug ("AlarmListWindow: Snooze Custom: %s for %d mins", a->message, mins);
            alarm_snooze (a, mins * 60);
        }
    }
}

/**
 * Update the snooze menu according to the applet's snooze_mins property
 */
void
alarm_list_window_snooze_menu_update (AlarmListWindow *list_window)
{
    AlarmApplet *applet = (AlarmApplet *)list_window->applet;
    GtkMenuShell *menu = GTK_MENU_SHELL(list_window->snooze_menu);
    gchar *target_name = g_strdup_printf ("snooze-menu-%d", applet->snooze_mins);

    const gchar *name;
    GtkMenuItem *item;
    GList *l = NULL;

    g_debug ("AlarmListWindow: menu_update to %d", applet->snooze_mins);

    block_list (menu->children, alarm_list_window_snooze_menu_activated);
    
    for (l = menu->children; l != NULL; l = l->next) {
        item = GTK_MENU_ITEM (l->data);
        name = gtk_widget_get_name (GTK_WIDGET (item));
        if (g_strcmp0 (name, target_name) == 0) {
            g_object_set (item, "active", TRUE, NULL);
            
            g_debug ("AlarmListWindow: menu_update to %s", name);
        }
    }

    unblock_list (menu->children, alarm_list_window_snooze_menu_activated);
    
    g_free (target_name);
}


void
alarm_list_window_snooze_menu_activate (GtkMenuItem *item, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
    AlarmListWindow *list_window = applet->list_window;
    gboolean active;

    guint mins = GPOINTER_TO_INT (g_object_get_data (item, "snooze"));
    g_object_get (item, "active", &active, NULL);

    g_debug ("%p Snooze for %d mins.. (active is %d).", item, mins, active);

}


