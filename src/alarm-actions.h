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


#ifndef ALARM_ACTIONS_H_
#define ALARM_ACTIONS_H_

#include "alarm-applet.h"

void
alarm_applet_actions_init (AlarmApplet *applet);

void
alarm_applet_actions_update_sensitive (AlarmApplet *applet);

void
alarm_action_update_enabled (AlarmApplet *applet);

void
alarm_action_edit (GtkAction *action, gpointer data);

void
alarm_action_delete (GtkAction *action, gpointer data);

void
alarm_action_enabled (GtkToggleAction *action, gpointer data);

void
alarm_action_stop (GtkAction *action, gpointer data);

void
alarm_action_stop_all (GtkAction *action, gpointer data);

void
alarm_action_new (GtkAction *action, gpointer data);

void
alarm_action_snooze (GtkAction *action, gpointer data);

void
alarm_action_snooze_all (GtkAction *action, gpointer data);

void
alarm_action_toggle_list_win (GtkAction *action, gpointer data);

void
alarm_action_quit (GtkAction *action, gpointer data);

void
alarm_action_toggle_autostart (GtkAction *action, gpointer data);

void
alarm_action_toggle_show_label (GtkAction *action, gpointer data);

void
alarm_action_toggle_time_format_12h (GtkAction *action, gpointer data);

#endif // ALARM_ACTIONS_H
