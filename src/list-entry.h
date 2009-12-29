/*
 * list-entry.h -- Simple data structure to hold name, data and icon.
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

#ifndef LISTENTRY_H_
#define LISTENTRY_H_

#include <string.h>
#include <glib.h>

#include "util.h"

G_BEGIN_DECLS

typedef struct {
	gchar *name;
	gchar *data;
	gchar *icon;
} AlarmListEntry;


AlarmListEntry *
alarm_list_entry_new (const gchar *name, const gchar *data, const gchar *icon);

void
alarm_list_entry_free (AlarmListEntry *e);

AlarmListEntry *
alarm_list_entry_new_file (const gchar *uri, gchar **mime_ret, GError **error);

GList *
alarm_list_entry_list_new (const gchar *dir_uri, const gchar *supported_types[]);

void
alarm_list_entry_list_free (GList **list);

G_END_DECLS

#endif /*LISTENTRY_H_*/
