/*
 * edit-alarm.h -- Alarm settings dialog
 * 
 * Copyright (C) 2007-2008 Johannes H. Jensen <joh@deworks.net>
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
 * 		Johannes H. Jensen <joh@deworks.net>
 */

#ifndef EDITALARM_H_
#define EDITALARM_H_

#include "alarm-applet.h"
#include "alarm.h"
#include "player.h"

typedef struct _AlarmSettingsDialog {
	
	AlarmApplet *applet;
	Alarm *alarm;
	
	/* Media player for previews */
	MediaPlayer *player;
	
	GtkWidget *dialog;
	GtkWidget *clock_toggle, *timer_toggle;
	GtkWidget *label_entry;
	GtkWidget *hour_spin, *min_spin, *sec_spin;
	
	/* Repeat */
	GtkWidget *repeat_expand;
	GtkWidget *repeat_label;
	GtkWidget *repeat_check[7];		/* Mon, tue, ..., sun check boxes */
	
	/* Snooze */
	GtkWidget *snooze_check;
	GtkWidget *snooze_spin;
	
	/* Notification */
	GtkWidget *notify_sound_radio;
	GtkWidget *notify_sound_box;
	GtkWidget *notify_sound_stock;
	GtkWidget *notify_sound_combo;
	GtkWidget *notify_sound_loop_check;
	GtkWidget *notify_sound_preview;
	
	GtkWidget *notify_app_radio;
	GtkWidget *notify_app_box;
	GtkWidget *notify_app_combo;
	GtkWidget *notify_app_command_box;
	GtkWidget *notify_app_command_entry;
	
	GtkWidget *notify_bubble_check;
	
} AlarmSettingsDialog;

void
display_edit_alarm_dialog (AlarmApplet *applet, Alarm *alarm);

void
alarm_settings_dialog_close (AlarmSettingsDialog *dialog);

#endif /*EDITALARM_H_*/
