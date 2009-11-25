/*
 * util.h -- Misc utilities
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

#ifndef UTIL_H_
#define UTIL_H_

#include <time.h>
#include <glib.h>

/**
 * Calculates the alarm timestamp given hour, min and secs.
 */
time_t
get_alarm_timestamp (guint hour, guint minute, guint second);

/**
 * Construct a Uppercased name of filename without the extension.
 */
gchar *
to_basename (const gchar *filename);

/*
 * Run Command
 */
gboolean
command_run (const gchar *command);

gboolean
is_executable_valid (gchar *executable);

/* Get full path of a data file */
char *
alarm_applet_get_data_path (const char *name);

#endif /*UTIL_H_*/
