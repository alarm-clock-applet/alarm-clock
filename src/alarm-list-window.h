// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * alarms-list.h -- Alarm list window
 *
 * Copyright (C) 2007-2008 Johannes H. Jensen <joh@pseudoberries.com>
 * Copyright (C) 2022 Tasos Sahanidis <code@tasossah.com>
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
    COLUMN_TRIGGERED,
    COLUMN_SHOW_ICON,
    ALARMS_N_COLUMNS,
} AlarmListColumn;

typedef enum {
    SORTID_TIME_REMAINING = 0,
} AlarmSortID;

struct _AlarmListWindow {
    AlarmApplet* applet;

    Alarm* selected_alarm;
    gboolean reordered; // Indicates that rows have just been reordered
    gboolean toggled;   // Indicates that an alarm has just been toggled

    GtkWindow* window;
    GtkListStore* model;
    GtkTreeView* tree_view;

    GtkWidget* new_button;
    GtkWidget* edit_button;
    GtkWidget* delete_button;
    GtkWidget* enable_button;
    GtkWidget* stop_button;
    GtkWidget* snooze_button;
    GtkWidget* snooze_menu;

    GdkPixbuf* alarm_icon;
    GdkPixbuf* timer_icon;
};

// #define TIME_COL_FORMAT "<span font='Bold 11'>%H:%M:%S</span>"
//  TODO: Does fixing the font size give a11y problems?
#define TIME_COL_CLOCK_FORMAT      "<span font='Bold 11'> %H:%M</span><span font='Bold 7'>:%S</span>"
#define TIME_COL_TIMER_FORMAT      "<span font='Bold 11'>-%H:%M</span><span font='Bold 7'>:%S</span>"
#define TIME_COL_REPEAT_FORMAT     "\n <sup>%s</sup>"
#define LABEL_COL_FORMAT           "%s"
#define LABEL_COL_TRIGGERED_FORMAT "<b>%s</b>"

#define CLOCK_FORMAT "%H:%M"
#define TIMER_FORMAT "-%H:%M"

#define DELETE_DIALOG_TITLE     _("Delete %s?")
#define DELETE_DIALOG_TEXT      _("<big>Delete %s <b>%s</b>?</big>")
#define DELETE_DIALOG_SECONDARY _("Are you sure you want to delete the %s labeled <b>%s</b> scheduled at <b>%s</b>? This action cannot be undone.")

AlarmListWindow* alarm_list_window_new(AlarmApplet* applet);

void alarm_list_window_show(AlarmListWindow* list_window);

void alarm_list_window_hide(AlarmListWindow* list_window);

void alarm_list_window_toggle(AlarmListWindow* list_window);

void alarm_list_window_alarm_add(AlarmListWindow* list_window, Alarm* alarm);

void alarm_list_window_alarm_update(AlarmListWindow* list_window, Alarm* alarm);

void alarm_list_window_alarm_remove(AlarmListWindow* list_window, Alarm* alarm);

void alarm_list_window_alarms_add(AlarmListWindow* list_window, GList* alarms);

gboolean alarm_list_window_find_alarm(GtkTreeModel* model, Alarm* alarm, GtkTreeIter* iter);

gboolean alarm_list_window_contains(AlarmListWindow* list_window, Alarm* alarm);

Alarm* alarm_list_window_get_selected_alarm(AlarmListWindow* list_window);

void alarm_list_request_resize(AlarmListWindow* list_window);

#endif /*ALARM_LIST_WINDOW_H_*/
