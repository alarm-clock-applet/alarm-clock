/*
 * util.c -- Misc utilities
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

#include <string.h>
#include <time.h>
#include <glib.h>
#include <glib-object.h>
#include "util.h"

/**
 * Calculates the alarm timestamp given hour, min and secs.
 */
time_t
get_alarm_timestamp (guint hour, guint minute, guint second)
{
	time_t now;
	struct tm *tm;
	
	time (&now);
	tm = localtime (&now);
	
	// Check if the alarm is for tomorrow
	if (hour < tm->tm_hour ||
		(hour == tm->tm_hour && minute < tm->tm_min) ||
		(hour == tm->tm_hour && minute == tm->tm_min && second < tm->tm_sec)) {
		
		g_debug("Alarm is for tomorrow.");
		tm->tm_mday++;
	}
	
	tm->tm_hour = hour;
	tm->tm_min = minute;
	tm->tm_sec = second;
	
	// DEBUG:
	char tmp[512];
	strftime (tmp, sizeof (tmp), "%c", tm);
	g_debug ("Alarm will trigger at %s", tmp);
	
	return mktime (tm);
}

/**
 * Construct a Uppercased name of filename without the extension.
 */
gchar *
to_basename (const gchar *filename)
{
	gint i, len;
	gchar *result;
	
	len = strlen (filename);
	// Remove extension
	for (i = len-1; i > 0; i--) {
		if (filename[i] == '.') {
			break;
		}
	}
	
	if (i == 0)
		// Extension not found
		i = len;
	
	result = g_strndup (filename, i);
	
	// Make first character Uppercase
	result[0] = g_utf8_strup (result, 1)[0];
	
	return result;
}

/*
 * Run Command
 */
gboolean
command_run (const gchar *command) {
	GError *err = NULL;
	
	if (!g_spawn_command_line_async (command, &err)) {
		g_critical ("Could not launch `%s': %s", command, err->message);
		g_error_free (err);
		
		return FALSE;
	}
	
	return TRUE;
}

gboolean
is_executable_valid (gchar *executable)
{
    gchar *path;

    path = g_find_program_in_path (executable);

    if (path) {
		g_free (path);
		return TRUE;
    }

    return FALSE;
}


/*
 * Get full path to a data file
 */
char *
alarm_applet_get_data_path (const char *name)
{
    char *filename;

#ifdef ALARM_CLOCK_RUN_IN_SOURCE_TREE
    /* Try the file in the source tree first */
    filename = g_build_filename ("..", "data", name, NULL);
    g_debug("filename: %s", filename);
    if (g_file_test (filename, G_FILE_TEST_EXISTS) == FALSE)
    {
        g_free (filename);
        /* Try the local file */
        filename = g_build_filename (ALARM_CLOCK_PKGDATADIR, name, NULL);

        if (g_file_test (filename, G_FILE_TEST_EXISTS) == FALSE)
        {
            g_free (filename);
            return NULL;
        }
    }
#else
    filename = g_build_filename (ALARM_CLOCK_PKGDATADIR, name, NULL);
#endif

    return filename;
}