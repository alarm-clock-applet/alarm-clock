// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * list-entry.h -- Simple data structure to hold name, data and icon.
 *
 * Copyright (C) 2007-2008 Johannes H. Jensen <joh@pseudoberries.com>
 * Copyright (C) 2022 Tasos Sahanidis <code@tasossah.com>
 */

#ifndef LISTENTRY_H_
#define LISTENTRY_H_

#include <string.h>
#include <glib.h>

#include "util.h"

G_BEGIN_DECLS

typedef struct {
    gchar* name;
    gchar* data;
    gchar* icon;
} AlarmListEntry;


AlarmListEntry* alarm_list_entry_new(const gchar* name, const gchar* data, const gchar* icon);

void alarm_list_entry_free(AlarmListEntry* e);

AlarmListEntry* alarm_list_entry_new_file(const gchar* uri, gchar** mime_ret, GError** error);

GList* alarm_list_entry_list_new(const gchar* dir_uri, const gchar* supported_types[]);

void alarm_list_entry_list_free(GList** list);

G_END_DECLS

#endif /*LISTENTRY_H_*/
