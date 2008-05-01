/*
 * alarms-list.h -- List alarms dialog
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

#ifndef ALARMSLIST_H_
#define ALARMSLIST_H_

#include "alarm-applet.h"
#include "alarm.h"

typedef enum {
	TYPE_COLUMN,
	ACTIVE_COLUMN,
	TIME_COLUMN,
	LABEL_COLUMN,
	PLAYING_COLUMN,
	ALARMS_N_COLUMNS
} AlarmColumn;


void
list_alarms_dialog_display (AlarmApplet *applet);

void
list_alarms_dialog_close (AlarmApplet *applet);

void
list_alarms_update (AlarmApplet *applet);

#endif /*ALARMSLIST_H_*/
