// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * ui.h - Alarm Clock applet UI routines
 *
 * Copyright (C) 2007-2008 Johannes H. Jensen <joh@pseudoberries.com>
 * Copyright (C) 2022 Tasos Sahanidis <code@tasossah.com>
 */

#ifndef UI_H_
#define UI_H_

#include "alarm-applet.h"
#include "alarm-list-window.h"
#include "alarm-settings.h"

void display_error_dialog(const gchar* message, const gchar* secondary_text, GtkWindow* parent);

void alarm_applet_label_update(AlarmApplet* applet);

void alarm_applet_update_tooltip(AlarmApplet* applet);

void alarm_applet_icon_update(AlarmApplet* applet);

void fill_combo_box(GtkComboBox* combo_box, GList* list, const gchar* custom_label);

void fill_combo_box_custom_list(GtkComboBox* combo_box, GList* list, const gchar* custom_label);

void alarm_applet_notification_show(AlarmApplet* applet, const gchar* summary, const gchar* body, const gchar* icon);

void alarm_applet_ui_init(AlarmApplet* applet);

GtkBuilder* alarm_applet_ui_load(const char* name, AlarmApplet* applet);

void alarm_applet_alarm_changed(GObject* object, GParamSpec* pspec, gpointer data);

void alarm_applet_alarm_triggered(Alarm* alarm, gpointer data);

void alarm_applet_alarm_cleared(Alarm* alarm, gpointer data);

void alarm_applet_status_update(AlarmApplet* applet);

void alarm_applet_menu_init(AlarmApplet* applet);

void media_player_error_cb(MediaPlayer* player, GError* err, gpointer data);

void alarm_applet_status_menu_edit_cb(GtkMenuItem* menuitem, gpointer user_data);

void alarm_applet_status_menu_prefs_cb(GtkMenuItem* menuitem, gpointer user_data);

void alarm_applet_status_menu_about_cb(GtkMenuItem* menuitem, gpointer user_data);


#endif /*UI_H_*/
