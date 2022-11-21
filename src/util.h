// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * util.h -- Misc utilities
 *
 * Copyright (C) 2007-2008 Johannes H. Jensen <joh@pseudoberries.com>
 * Copyright (C) 2022 Tasos Sahanidis <code@tasossah.com>
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <time.h>
#include <glib.h>

#include <config.h>

/**
 * Calculates the alarm timestamp given hour, min and secs.
 */
time_t get_alarm_timestamp(guint hour, guint minute, guint second);

/**
 * Construct a Uppercased name of filename without the extension.
 */
gchar* to_basename(const gchar* filename);

/*
 * Run Command
 */
gboolean command_run(const gchar* command);

gboolean is_executable_valid(gchar* executable);

/* Get full path of a data file */
gchar* alarm_applet_get_data_path(const char* name);

guint block_signal_handlers_by_name(gpointer instance, const gchar* signal_name);

guint unblock_signal_handlers_by_name(gpointer instance, const gchar* signal_name);

guint block_list(GList* instances, gpointer func);

guint unblock_list(GList* instances, gpointer func);

#define BLOCK(instance, func) g_signal_handlers_block_matched((instance), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (func), NULL)

#define UNBLOCK(instance, func) g_signal_handlers_unblock_matched((instance), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (func), NULL)

#endif /*UTIL_H_*/
