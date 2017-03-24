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

#include <config.h>

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

guint
block_signal_handlers_by_name (gpointer instance, const gchar *signal_name);

guint
unblock_signal_handlers_by_name (gpointer instance, const gchar *signal_name);

guint
block_list (GList *instances, gpointer func);

guint
unblock_list (GList *instances, gpointer func);

#define BLOCK(instance, func)   \
    g_signal_handlers_block_matched ((instance),            \
                                     G_SIGNAL_MATCH_FUNC,   \
                                     0, 0, NULL, (func), NULL)

#define UNBLOCK(instance, func)   \
    g_signal_handlers_unblock_matched ((instance),            \
                                       G_SIGNAL_MATCH_FUNC,   \
                                       0, 0, NULL, (func), NULL)
                                       
/* Converts hour from 12 hour format (am/pm) to 24 hour format */
#define HOUR_12_TO_24(hour, is_pm) \
    is_pm ? (hour != 12 ? hour + 12 : hour) : (hour == 12 ? 0 : hour)

/* Converts hour from 24 hour format to 12 hour format (am/pm) */
#define HOUR_24_TO_12(hour) \
    hour > 12 ? hour - 12 : (hour < 1 ? 12 : hour);

/* True if the hour (in 24 hour format) is in the PM, false if AM */
# define IS_HOUR_PM(hour) \
    hour > 12 ? TRUE : (hour < 12 ? FALSE : TRUE)

#endif /*UTIL_H_*/
