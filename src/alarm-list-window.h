/*
 * alarms-list.h -- Alarm list window
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

#ifndef ALARM_LIST_WINDOW_H_
#define ALARM_LIST_WINDOW_H_

#include <gtk/gtk.h>

typedef struct _AlarmListWindow AlarmListWindow;

#include "alarm-applet.h"
#include "alarm.h"

typedef enum {
    COLUMN_ALARM = 0,
    COLUMN_TYPE,
    COLUMN_TIME,
    COLUMN_LABEL,
    COLUMN_ACTIVE,
    ALARMS_N_COLUMNS
} AlarmListColumn;

typedef enum {
    SORTID_TIME_REMAINING = 0,
} AlarmSortID;

struct _AlarmListWindow {
	AlarmApplet *applet;
    
    Alarm *selected_alarm;
    gboolean reordered;         // Indicates that rows have just been reordered
    gboolean toggled;           // Indicates that an alarm has just been toggled
	
	GtkWindow *window;
	GtkListStore *model;
	GtkTreeView *tree_view;

    GtkWidget *new_button;
    GtkWidget *edit_button;
    GtkWidget *delete_button;
    GtkWidget *enable_button;
    GtkWidget *stop_button;
    GtkWidget *snooze_button;
    GtkWidget *snooze_menu;

    GtkAction *snooze_action;
};

//#define TIME_COL_FORMAT "<span font='Bold 11'>%H:%M:%S</span>"
// TODO: Does fixing the font size give a11y problems?
#define TIME_COL_CLOCK_FORMAT "<span font='Bold 11'> %H:%M</span><span font='Bold 7'>:%S</span>"
#define TIME_COL_TIMER_FORMAT "<span font='Bold 11'>-%H:%M</span><span font='Bold 7'>:%S</span>"
#define TIME_COL_REPEAT_FORMAT "\n <sup>%s</sup>"
#define LABEL_COL_FORMAT "%s"

#define CLOCK_FORMAT "%H:%M"
#define TIMER_FORMAT "-%H:%M"

#define DELETE_DIALOG_TITLE     _("Delete %s?")
#define DELETE_DIALOG_TEXT      _("<big>Delete %s <b>%s</b>?</big>")
#define DELETE_DIALOG_SECONDARY _("Are you sure you want to delete the %s labeled <b>%s</b> scheduled at <b>%s</b>? This action cannot be undone.")

AlarmListWindow *
alarm_list_window_new (AlarmApplet *applet);

void
alarm_list_window_show (AlarmListWindow *list_window);

void
alarm_list_window_hide (AlarmListWindow *list_window);

void
alarm_list_window_toggle (AlarmListWindow *list_window);

void
alarm_list_window_alarm_add (AlarmListWindow *list_window, Alarm *alarm);

void
alarm_list_window_alarm_update (AlarmListWindow *list_window, Alarm *alarm);

void
alarm_list_window_alarm_remove (AlarmListWindow *list_window, Alarm *alarm);

void
alarm_list_window_alarms_add (AlarmListWindow *list_window, GList *alarms);

#endif /*ALARM_LIST_WINDOW_H_*/
