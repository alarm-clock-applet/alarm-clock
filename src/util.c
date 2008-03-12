/*
 * ALARM APPLET Utilities
 * 
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

