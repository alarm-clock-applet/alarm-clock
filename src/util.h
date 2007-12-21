/*
 * ALARM APPLET Utilities
 * 
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

#endif /*UTIL_H_*/
