// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * util.c -- Misc utilities
 *
 * Copyright (C) 2007-2008 Johannes H. Jensen <joh@pseudoberries.com>
 * Copyright (C) 2022 Tasos Sahanidis <code@tasossah.com>
 */

#include <string.h>
#include <time.h>
#include <glib.h>
#include <glib-object.h>
#include "util.h"

/**
 * Calculates the alarm timestamp given hour, min and secs.
 */
time_t get_alarm_timestamp(guint hour, guint minute, guint second)
{
    time_t now;
    struct tm* tm;

    time(&now);
    tm = localtime(&now);

    // Check if the alarm is for tomorrow
    if(hour < tm->tm_hour || (hour == tm->tm_hour && minute < tm->tm_min) || (hour == tm->tm_hour && minute == tm->tm_min && second < tm->tm_sec)) {

        g_debug("Alarm is for tomorrow.");
        tm->tm_mday++;
    }

    tm->tm_hour = hour;
    tm->tm_min = minute;
    tm->tm_sec = second;

    // DEBUG:
    char tmp[512];
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-y2k"
#endif
    strftime(tmp, sizeof(tmp), "%c", tm);
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
    g_debug("Alarm will trigger at %s", tmp);

    return mktime(tm);
}

/**
 * Construct a Uppercased name of filename without the extension.
 */
gchar* to_basename(const gchar* filename)
{
    gint i, len;
    gchar* result;

    len = strlen(filename);
    // Remove extension
    for(i = len - 1; i > 0; i--) {
        if(filename[i] == '.') {
            break;
        }
    }

    if(i == 0)
        // Extension not found
        i = len;

    result = g_strndup(filename, i);

    // Make first character Uppercase
    result[0] = g_utf8_strup(result, 1)[0];

    return result;
}

/*
 * Run Command
 */
gboolean command_run(const gchar* command)
{
    GError* err = NULL;

    if(!g_spawn_command_line_async(command, &err)) {
        g_critical("Could not launch `%s': %s", command, err->message);
        g_error_free(err);

        return FALSE;
    }

    return TRUE;
}

gboolean is_executable_valid(gchar* executable)
{
    gchar* path;

    path = g_find_program_in_path(executable);

    if(path) {
        g_free(path);
        return TRUE;
    }

    return FALSE;
}


/*
 * Get full path to a data file
 */
gchar* alarm_applet_get_data_path(const char* name)
{
    gchar* filename;

    /* Try the file in the source tree first */
    filename = g_build_filename("..", "data", name, NULL);
    g_debug("filename: %s", filename);
    if(g_file_test(filename, G_FILE_TEST_EXISTS) == FALSE) {
        g_free(filename);
        /* Try the local file */
        filename = g_build_filename(ALARM_CLOCK_PKGDATADIR, name, NULL);

        if(g_file_test(filename, G_FILE_TEST_EXISTS) == FALSE) {
            g_free(filename);
            return NULL;
        }
    }

    return filename;
}

/**
 * Block signal handlers by signal name
 */
guint block_signal_handlers_by_name(gpointer instance, const gchar* signal_name)
{
    guint signal_id;

    signal_id = g_signal_lookup(signal_name, G_OBJECT_TYPE(instance));

    g_warn_if_fail(signal_id);
    g_return_val_if_fail(signal_id, 0);

    return g_signal_handlers_block_matched(instance, G_SIGNAL_MATCH_ID, signal_id, 0, NULL, NULL, NULL);
}

/**
 * Unblock signal handlers by signal name
 */
guint unblock_signal_handlers_by_name(gpointer instance, const gchar* signal_name)
{
    guint signal_id;

    signal_id = g_signal_lookup(signal_name, G_OBJECT_TYPE(instance));

    g_warn_if_fail(signal_id);
    g_return_val_if_fail(signal_id, 0);

    return g_signal_handlers_unblock_matched(instance, G_SIGNAL_MATCH_ID, signal_id, 0, NULL, NULL, NULL);
}

guint block_list(GList* instances, gpointer func)
{
    guint blocked = 0;

    for(GList* l = instances; l != NULL; l = l->next) {
        blocked += BLOCK(l->data, func);
    }

    return blocked;
}

guint unblock_list(GList* instances, gpointer func)
{
    guint unblocked = 0;

    for(GList* l = instances; l != NULL; l = l->next) {
        unblocked += UNBLOCK(l->data, func);
    }

    return unblocked;
}
