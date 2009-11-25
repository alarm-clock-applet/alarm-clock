/*
 * ui.h - Alarm Clock applet UI routines
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

#ifndef UI_H_
#define UI_H_

#include "alarm-applet.h"

#ifdef HAVE_LIBNOTIFY
#include <libnotify/notify.h>
#endif

//#define ORIENT_IS_HORIZONTAL(o)		((o) == PANEL_APPLET_ORIENT_UP || (o) == PANEL_APPLET_ORIENT_DOWN)
//#define IS_HORIZONTAL(papplet)		(ORIENT_IS_HORIZONTAL (panel_applet_get_orient (papplet)))


void
display_error_dialog (const gchar *message, const gchar *secondary_text, GtkWindow *parent);

void
alarm_applet_label_update (AlarmApplet *applet);

void
alarm_applet_update_tooltip (AlarmApplet *applet);

void
alarm_applet_icon_update (AlarmApplet *applet);

/*
 * Shamelessly stolen from gnome-da-capplet.c
 */
void
fill_combo_box (GtkComboBox *combo_box, GList *list, const gchar *custom_label);

gboolean
alarm_applet_notification_display (AlarmApplet *applet, Alarm *alarm);

gboolean
alarm_applet_notification_close (AlarmApplet *applet);

void
alarm_applet_ui_init (AlarmApplet *applet);

void
alarm_applet_menu_init (AlarmApplet *applet);

void
media_player_error_cb (MediaPlayer *player, GError *err, GtkWindow *parent);

#endif /*UI_H_*/
