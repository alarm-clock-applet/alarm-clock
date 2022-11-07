// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * alarm-actions.h -- Alarm actions
 *
 * Copyright (C) 2010 Johannes H. Jensen <joh@pseudoberries.com>
 * Copyright (C) 2022 Tasos Sahanidis <code@tasossah.com>
 */

#ifndef ALARM_ACTIONS_H_
#define ALARM_ACTIONS_H_

#include "alarm-applet.h"

void alarm_applet_actions_init(AlarmApplet* applet);

void alarm_applet_actions_update_sensitive(AlarmApplet* applet);

void alarm_action_update_enabled(AlarmApplet* applet);

void alarm_action_edit(GSimpleAction* action, GVariant* parameter, gpointer data);

void alarm_action_delete(GSimpleAction* action, GVariant* parameter, gpointer data);

void alarm_action_enable(GSimpleAction* action, GVariant* parameter, gpointer data);

void alarm_action_stop(GSimpleAction* action, GVariant* parameter, gpointer data);

void alarm_action_stop_all(GSimpleAction* action, GVariant* parameter, gpointer data);

void alarm_action_new(GSimpleAction* action, GVariant* parameter, gpointer data);

void alarm_action_snooze(GSimpleAction* action, GVariant* parameter, gpointer data);

void alarm_action_snooze_all(GSimpleAction* action, GVariant* parameter, gpointer data);

void alarm_action_toggle_list_win(GSimpleAction* action, GVariant* parameter, gpointer data);

void alarm_action_quit(GSimpleAction* action, GVariant* parameter, gpointer data);

void alarm_action_toggle_autostart(GSimpleAction* action, GVariant* parameter, gpointer data);

void alarm_action_toggle_show_label(GSimpleAction* action, GVariant* parameter, gpointer data);

#endif // ALARM_ACTIONS_H
