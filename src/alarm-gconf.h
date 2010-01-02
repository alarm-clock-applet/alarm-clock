/*
 * alarm-gconf.h -- GConf routines
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

#ifndef ALARM_GCONF_H_
#define ALARM_GCONF_H_

#include <time.h>
//#include <panel-applet-gconf.h>

#include "alarm-applet.h"

G_BEGIN_DECLS

/* Global GConf keys */
#define KEY_SHOW_NOTIFY     "show_notification"

#define N_GCONF_PREFS		2	/* + global gconf listener */

/* Failsafe defaults for use when the schema isn't found
 * or doesn't provide sensible defaults */
#define DEF_SHOW_NOTIFY     TRUE

/*
 * Init
 */
void
alarm_applet_gconf_init (AlarmApplet *applet);

void
alarm_applet_gconf_load (AlarmApplet *applet);

G_END_DECLS

#endif /*ALARM_GCONF_H_*/
