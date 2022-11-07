// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * alarm-settings.h -- Alarm settings dialog
 *
 * Copyright (C) 2007-2008 Johannes H. Jensen <joh@pseudoberries.com>
 * Copyright (C) 2022 Tasos Sahanidis <code@tasossah.com>
 */

#ifndef EDITALARM_H_
#define EDITALARM_H_

#include "alarm.h"
#include "player.h"

typedef struct _AlarmSettingsDialog AlarmSettingsDialog;

#include "alarm-applet.h"

struct _AlarmSettingsDialog {

    AlarmApplet* applet;
    Alarm* alarm;

    /* Media player for previews */
    MediaPlayer* player;

    GtkWidget* dialog;
    GtkWidget *clock_toggle, *timer_toggle;
    GtkWidget* label_entry;
    GtkWidget *hour_spin, *min_spin, *sec_spin;

    /* Repeat */
    GtkWidget* repeat_expand;
    GtkWidget* repeat_label;
    GtkWidget* repeat_check[7]; /* Mon, tue, ..., sun check boxes */

    /* Notification */
    GtkWidget* notify_sound_radio;
    GtkWidget* notify_sound_box;
    GtkWidget* notify_sound_stock;
    GtkWidget* notify_sound_combo;
    GtkWidget* notify_sound_loop_check;
    GtkWidget* notify_sound_preview;

    GtkWidget* notify_app_radio;
    GtkWidget* notify_app_box;
    GtkWidget* notify_app_combo;
    GtkWidget* notify_app_command_box;
    GtkWidget* notify_app_command_entry;
};

AlarmSettingsDialog* alarm_settings_dialog_new(AlarmApplet* applet);

void alarm_settings_dialog_show(AlarmSettingsDialog* dialog, Alarm* alarm);

void display_edit_alarm_dialog(AlarmApplet* applet, Alarm* alarm);

void alarm_settings_dialog_close(AlarmSettingsDialog* dialog);

gboolean alarm_settings_output_time(GtkSpinButton* spin, gpointer data);

void alarm_settings_sound_preview(GtkButton* button, gpointer data);

void alarm_settings_dialog_response(GtkDialog* dialog, gint rid, gpointer data);

#endif /*EDITALARM_H_*/
