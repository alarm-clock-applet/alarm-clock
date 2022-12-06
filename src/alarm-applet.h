// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * alarm-applet.h -- Alarm Clock applet bootstrap
 *
 * Copyright (C) 2007-2008 Johannes H. Jensen <joh@pseudoberries.com>
 * Copyright (C) 2022 Tasos Sahanidis <code@tasossah.com>
 */

#ifndef ALARMAPPLET_H_
#define ALARMAPPLET_H_

#include <string.h>
#include <sys/time.h>
#include <time.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gst/gst.h>

#include <config.h>

#include <libayatana-appindicator/app-indicator.h>

G_BEGIN_DECLS

typedef struct _AlarmApplet AlarmApplet;

extern GHashTable* app_command_map;

void alarm_applet_label_update(AlarmApplet* applet);
void alarm_applet_clear_alarms(AlarmApplet* applet);

#include "alarm.h"
#include "prefs.h"
#include "alarm-gsettings.h"
#include "player.h"
#include "util.h"
#include "list-entry.h"
#include "ui.h"

#define ALARM_NAME       "Alarm Clock"
#define ALARM_ICON       "alarm-clock"
#define TIMER_ICON       "alarm-timer"
#define TRIGGERED_ICON   "alarm-clock-triggered"
#define ALARM_STD_SNOOZE 9

typedef enum {
    LABEL_TYPE_INVALID = 0,
    LABEL_TYPE_TIME,
    LABEL_TYPE_REMAIN,
} LabelType;

typedef enum {
    UNIQUE_STOP_ALL = 1,
    UNIQUE_SNOOZE_ALL,
} UniqueCustomCommandID;

struct _AlarmApplet {
    /* Gtk App */
    GtkApplication* application;

    /* User Interface */
    GtkBuilder* ui;

    /* App Indicator */
    AppIndicator* app_indicator;

    /* Status menu */
    GtkWidget* status_menu;

    /* Alarms */
    GList* alarms;
    guint n_triggered; // Number of triggered alarms

    /* Sounds & apps list */
    GList* sounds;
    GList* apps;

    /* List-alarms UI */
    AlarmListWindow* list_window;

    /* Alarm settings dialog */
    AlarmSettingsDialog* settings_dialog;

    /* Preferences */
    GtkDialog* prefs_dialog;
    GtkWidget* prefs_autostart_check;

    guint snooze_mins;

    GSimpleAction* action_edit;
    GSimpleAction* action_delete;
    GSimpleAction* action_stop;
    GSimpleAction* action_snooze;
    GSimpleAction* action_enable;

    // Global actions
    GSimpleAction* action_snooze_all;
    GSimpleAction* action_stop_all;
    GSimpleAction* action_toggle_list_win;
    GSimpleAction* action_toggle_autostart;
    GSimpleAction* action_toggle_show_label;

    // Args
    gboolean hidden; // Start hidden

    // GSettings
    GSettings* settings_global;
};

void alarm_applet_sounds_load(AlarmApplet* applet);

void alarm_applet_apps_load(AlarmApplet* applet);

void alarm_applet_alarms_load(AlarmApplet* applet);

void alarm_applet_alarms_add(AlarmApplet* applet, Alarm* alarm);

void alarm_applet_alarms_remove_and_delete(AlarmApplet* applet, Alarm* alarm);

guint alarm_applet_alarms_snooze(AlarmApplet* applet);

guint alarm_applet_alarms_stop(AlarmApplet* applet);

void alarm_applet_alarm_snooze(AlarmApplet* applet, Alarm* alarm);

void alarm_applet_alarm_stop(AlarmApplet* applet, Alarm* alarm);

void alarm_applet_destroy(AlarmApplet* applet);

void alarm_applet_request_resize(AlarmApplet* applet);

G_END_DECLS

#endif /*ALARMAPPLET_H_*/
