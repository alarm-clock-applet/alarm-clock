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
#include "alarm-actions.h"
#include "prefs.h"

gboolean
alarm_list_window_delete_event (GtkWidget *window, GdkEvent *event, gpointer data);

static void
alarm_list_window_selection_changed (GtkTreeSelection *, gpointer);

static gboolean
alarm_list_window_update_timer (gpointer);

static gint
alarm_list_window_sort_iter_compare (GtkTreeModel *model, 
                                     GtkTreeIter *a, GtkTreeIter *b,
                                     gpointer data);

void
alarm_list_window_rows_reordered (GtkTreeModel *model,
                                  GtkTreePath  *path,
                                  GtkTreeIter  *iter,
                                  gpointer      arg3,
                                  gpointer      data);

void
alarm_list_window_enable_toggled (GtkCellRendererToggle *cell_renderer,
                                  gchar *path,
                                  gpointer data);

void
alarm_list_window_snooze_menu_activated (GtkMenuItem *item, gpointer data);

void
alarm_list_window_snooze_menu_custom_activated (GtkMenuItem *menuitem, gpointer data);

void
alarm_list_window_snooze_menu_update (AlarmListWindow *list_window);

static void
alarm_list_window_update_row (AlarmListWindow *list_window, GtkTreeIter *iter);

/**
 * Create a new Alarm List Window
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

    list_window->new_button = GTK_WIDGET (gtk_builder_get_object (builder, "new-button"));
    list_window->edit_button = GTK_WIDGET (gtk_builder_get_object (builder, "edit-button"));
    list_window->delete_button = GTK_WIDGET (gtk_builder_get_object (builder, "delete-button"));
    list_window->enable_button = GTK_WIDGET (gtk_builder_get_object (builder, "enable-button"));
    list_window->stop_button = GTK_WIDGET (gtk_builder_get_object (builder, "stop-button"));
    list_window->snooze_button = GTK_WIDGET (gtk_builder_get_object (builder, "snooze-button"));
    list_window->snooze_menu = GTK_WIDGET (gtk_builder_get_object (builder, "snooze-menu"));
    
    // Set up window accelerator group
    list_window->accel_group = gtk_accel_group_new ();
    gtk_window_add_accel_group (list_window->window, list_window->accel_group);
    
    // Connect some signals
    selection = gtk_tree_view_get_selection (list_window->tree_view);
    g_signal_connect (selection, "changed", 
                      G_CALLBACK (alarm_list_window_selection_changed), applet);

    // Update view every second for pretty countdowns
    g_timeout_add (500, (GSourceFunc) alarm_list_window_update_timer, applet);

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

/**
 * Show and present list window
 */
void
alarm_list_window_show (AlarmListWindow *list_window)
{
    // Let the WM decide the initial position of the window (probably not 0,0)
    gboolean first_time = !gtk_widget_get_realized (GTK_WIDGET (list_window->window));

    gtk_window_present_with_time (list_window->window, gtk_get_current_event_time());
    if (!first_time) {
        gtk_window_move (list_window->window, list_window->window_pos_x, list_window->window_pos_y);
    }
}

/**
 * Hide list window
 */
void
alarm_list_window_hide (AlarmListWindow *list_window)
{
	gtk_window_get_position (list_window->window, &(list_window->window_pos_x), &(list_window->window_pos_y));
	gtk_widget_hide (GTK_WIDGET (list_window->window));
}

/**
 * Toggle visibility of list window
 */
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
 * Delete-event handler for list-window
 */
gboolean
alarm_list_window_delete_event (GtkWidget *window, GdkEvent *event, gpointer data)
{
	AlarmApplet *applet = (AlarmApplet *)data;

	gtk_action_activate (GTK_ACTION (applet->action_toggle_list_win));

	return TRUE;
}

//
// ALARM LIST MODEL:
//

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
    return alarm_list_window_find_alarm (GTK_TREE_MODEL (list_window->model), alarm, NULL);
}

/**
 * Update the row in the list at the position specified by iter
 */
static void
alarm_list_window_update_row (AlarmListWindow *list_window, GtkTreeIter *iter)
{
    GtkTreeModel *model = GTK_TREE_MODEL (list_window->model);
    Alarm *a;

    gchar tmp[200];
    gchar *tmp2;
    struct tm *tm;
    
    const gchar *type_col;
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
    
    gboolean time_format_12h = prefs_time_format_12h_get (list_window->applet);

    if (a->type == ALARM_TYPE_CLOCK) {
        type_col = ALARM_ICON;
        // show 12 hour format if enabled and not counting down
        if (time_format_12h && !a->active)
            strftime(tmp, sizeof(tmp), TIME_COL_CLOCK_FORMAT_12HR, tm);
        else
            strftime(tmp, sizeof(tmp), TIME_COL_CLOCK_FORMAT, tm);
    } else {
        type_col = TIMER_ICON;
        strftime(tmp, sizeof(tmp), TIME_COL_TIMER_FORMAT, tm);
    }

    // Create time column
    time_col = g_string_new (tmp);
    if (a->type == ALARM_TYPE_CLOCK && a->repeat != ALARM_REPEAT_NONE) {
        tmp2 = alarm_repeat_to_pretty (a->repeat);
        g_string_append_printf (time_col, TIME_COL_REPEAT_FORMAT, tmp2);
        g_free (tmp2);
    }
    
    // Create label column
    tmp2 = g_markup_escape_text (a->message, -1);
    if (a->triggered) {
        label_col = g_strdup_printf (LABEL_COL_TRIGGERED_FORMAT, tmp2);
    } else {
        label_col = g_strdup_printf (LABEL_COL_FORMAT, tmp2);
    }
    g_free (tmp2);
    
	gtk_list_store_set (GTK_LIST_STORE (model), iter,
                        COLUMN_TYPE, type_col,
                        COLUMN_TIME, time_col->str,
                        COLUMN_LABEL, label_col,
                        COLUMN_ACTIVE, a->active,
                        COLUMN_TRIGGERED, a->triggered,
                        -1);

    // Restore icon visibility when an alarm is cleared / snoozed
    if (!a->triggered) {
        gtk_list_store_set (GTK_LIST_STORE (model), iter, COLUMN_SHOW_ICON, TRUE, -1);
    }

    
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

    if (alarm_list_window_find_alarm (GTK_TREE_MODEL (list_window->model), alarm, &iter)) {
        alarm_list_window_update_row (list_window, &iter);
    } else {
        g_warning ("AlarmListWindow alarm_update: Could not find alarm %p", alarm);
    }
}

/**
 * Remove alarm from the list window
 */
void
alarm_list_window_alarm_remove (AlarmListWindow *list_window, Alarm *alarm)
{
    GtkTreeIter iter;

    if (alarm_list_window_find_alarm (GTK_TREE_MODEL (list_window->model), alarm, &iter)) {
        gtk_list_store_remove (list_window->model, &iter);
    } else {
        g_warning ("AlarmListWindow alarm_remove: Could not find alarm %p", alarm);
    }
}

/**
 * Add several alarms to the list window
 */
void
alarm_list_window_alarms_add (AlarmListWindow *list_window, GList *alarms)
{
    AlarmApplet *applet = list_window->applet;
    
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
    Alarm *a;
    gboolean show_icon;
    gboolean valid;

    valid = gtk_tree_model_get_iter_first (model, &iter);

    while (valid) {
        alarm_list_window_update_row (applet->list_window, &iter);
        
        gtk_tree_model_get (model, &iter, 
                            COLUMN_ALARM, &a,
                            COLUMN_SHOW_ICON, &show_icon, -1);

        // Blink icon on triggered alarms
        if (a->triggered) {
            gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_SHOW_ICON, !show_icon, -1);
        }
        
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
// TREE VIEW:
//

/**
 * Get the selected alarm
 */
Alarm *
alarm_list_window_get_selected_alarm (AlarmListWindow *list_window)
{
    GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	Alarm *a;

    g_assert (list_window);
	
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
 * Selection changed in tree view
 *
 * Here we update the associated actions
 */
void
alarm_list_window_selection_changed (GtkTreeSelection *selection, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
    AlarmListWindow *list_window = applet->list_window;
    Alarm *a = list_window->selected_alarm;

    // If rows have just been reordered, retain the selected row
    // But only if the reordering was triggered by a click on the toggle cell
    if (list_window->reordered && list_window->toggled) {
        list_window->reordered = FALSE;
        list_window->toggled = FALSE;

        alarm_list_window_select_alarm (list_window, list_window->selected_alarm);
        return;
    }

    // Reset reordered and toggled flags
    list_window->reordered = FALSE;
    list_window->toggled = FALSE;
    
    // Update actions
    alarm_applet_actions_update_sensitive (applet);
    alarm_action_update_enabled (applet);
    
    // Update selected alarm (might be NULL)
    list_window->selected_alarm = alarm_list_window_get_selected_alarm (list_window);

    if (list_window->selected_alarm) {
        // Update snooze button menu
        if (list_window->selected_alarm->type == ALARM_TYPE_CLOCK) {
            // We always snooze for 9 mins on alarm clocks
            gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (list_window->snooze_button), NULL);
        } else {
            // Allow custom snooze mins
            gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (list_window->snooze_button), list_window->snooze_menu);
        }
    }
    
    
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
        
        // Activate the enabled action
        gtk_action_activate (GTK_ACTION (applet->action_enabled));
    }
}

/**
 * Rows reordered callback
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

    // Return if list_window is not set. This happens during initialization.
    if (!list_window)
        return;

    // Signal to selection_changed that the rows have been reordered.
    list_window->reordered = TRUE;
}


//
// TOOLBAR:
//

/**
 * Snooze menu item activated
 */
void
alarm_list_window_snooze_menu_activated (GtkMenuItem *menuitem,
                                         gpointer          data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
    gchar **parts;
    guint i;
    guint mins;

//    g_debug ("AlarmListWindow: snooze-menu activated %s to %d", 
//        gtk_menu_item_get_label (menuitem), gtk_check_menu_item_get_active (menuitem));

    if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem))) {
        // Determine #mins from the name of the menu item (hackish)
        // Assumes name follows the format {foo}-{mins}
        parts = g_strsplit (gtk_buildable_get_name (GTK_BUILDABLE (menuitem)), "-", 0);
        for (i = 0; parts[i] != NULL; i++)
            // Loop to the last element
            ;
        
        mins = g_strtod (parts[i-1], NULL);
        
        g_debug ("AlarmListWindow: snooze-menu activated: Snooze for %d mins!", mins);

        applet->snooze_mins = mins;

        gtk_action_activate (applet->action_snooze);
    }
}

/**
 * Snooze menu custom item activated
 */
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
    
    dialog = GTK_WIDGET (gtk_builder_get_object (applet->ui, "snooze-dialog"));
    spin = GTK_WIDGET (gtk_builder_get_object (applet->ui, "snooze-spin"));

    // Run dialog, hide for later use
	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_hide (GTK_WIDGET (dialog));

	if (response == GTK_RESPONSE_OK) {
        mins = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin));
        if ((a = alarm_list_window_get_selected_alarm (list_window))) {
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
        name = gtk_buildable_get_name (GTK_BUILDABLE (item));
        if (g_strcmp0 (name, target_name) == 0) {
            g_object_set (item, "active", TRUE, NULL);
            
            g_debug ("AlarmListWindow: menu_update to %s", name);
        }
    }

    unblock_list (menu->children, alarm_list_window_snooze_menu_activated);
    
    g_free (target_name);
}
