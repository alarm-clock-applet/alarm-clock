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

#endif // ALARM_ACTIONS_H