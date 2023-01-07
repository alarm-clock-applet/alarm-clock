// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * list-entry.c -- Simple data structure to hold name, data and icon.
 *
 * Copyright (C) 2007-2008 Johannes H. Jensen <joh@pseudoberries.com>
 * Copyright (C) 2022 Tasos Sahanidis <code@tasossah.com>
 */

#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "list-entry.h"
#include "util.h"

void alarm_list_entry_free(AlarmListEntry* e)
{
    g_free(e->data);
    g_free(e->name);
    g_free(e->icon);
    g_free(e);
}

AlarmListEntry* alarm_list_entry_new_file(const gchar* uri, gchar** mime_ret, GError** error)
{
    AlarmListEntry* entry;
    GError* new_error = NULL;
    GFileInfo* info;
    GFile* file;

    file = g_file_new_for_uri(uri);
    info = g_file_query_info(file, "standard::content-type,standard::icon", G_FILE_QUERY_INFO_NONE, NULL, &new_error);

    if(new_error != NULL) {
        // g_warning ("Could not open uri: %s", uri);
        if(error)
            *error = new_error;
        else
            g_error_free(new_error);
        return NULL;
    }

    entry = g_new(AlarmListEntry, 1);
    entry->data = g_strdup(uri);
    entry->name = g_file_get_basename(file);
    entry->icon = g_icon_to_string(g_file_info_get_icon(info));

    if(mime_ret != NULL)
        *mime_ret = g_strdup(g_file_info_get_content_type(info));

    g_object_unref(info);
    g_object_unref(file);

    return entry;
}

static inline gboolean is_ignored(const gchar* const name, const gchar* const ignore[])
{
    if(!ignore)
        return FALSE;

    for(gint j = 0; ignore[j] != NULL; j++) {
        if(!strncmp(name, ignore[j], strlen(ignore[j])))
            return TRUE;
    }
    return FALSE;
}

GList* alarm_list_entry_list_new(GList* flist, const gchar* dir_uri, const gchar* supported_types[], const gchar* const ignore[])
{
    GError* error = NULL;
    GFile* dir;
    GFileEnumerator* result;
    GFileInfo* info;

    AlarmListEntry* entry;
    const gchar* mime;
    gboolean valid;
    gint i;

    dir = g_file_new_for_uri(dir_uri);
    result = g_file_enumerate_children(dir,
                                       "standard::type,standard::content-type,"
                                       "standard::icon,standard::name",
                                       G_FILE_QUERY_INFO_NONE, NULL, &error);

    if(error) {
        g_critical("Could not open directory: %s", dir_uri);
        g_error_free(error);
        return NULL;
    }

    g_debug("Loading files in %s ...", dir_uri);

    while((info = g_file_enumerator_next_file(result, NULL, NULL))) {
        // g_debug ("-- %s", g_file_info_get_name (info));
        if(g_file_info_get_file_type(info) == G_FILE_TYPE_REGULAR) {
            mime = g_file_info_get_content_type(info);
            // g_debug (" [ regular file: MIME: %s ]", mime);

            valid = TRUE;
            if(supported_types != NULL) {
                valid = FALSE;
                for(i = 0; supported_types[i] != NULL; i++) {
                    if(strstr(mime, supported_types[i]) != NULL && !is_ignored(g_file_info_get_name(info), ignore)) {
                        // MATCH
                        // g_debug (" [ MATCH ]");
                        valid = TRUE;
                        break;
                    }
                }
            }

            if(valid) {
                entry = g_new(AlarmListEntry, 1);
                entry->name = g_strdup(g_file_info_get_name(info));
                entry->data = g_strdup_printf("%s/%s", dir_uri, entry->name);
                entry->icon = g_icon_to_string(g_file_info_get_icon(info));

                // g_debug ("Icon found: %s", entry->icon);
                flist = g_list_append(flist, entry);
            }
        }
        g_object_unref(info);
    }

    g_file_enumerator_close(result, NULL, NULL);
    g_object_unref(result);
    g_object_unref(dir);

    return flist;
}

void alarm_list_entry_list_free(GList** list)
{
    GList* l;
    AlarmListEntry* e;

    g_debug("Alarm_file_entry_list_free (%p => %p)", list, *list);

    // Free data
    for(l = *list; l; l = l->next) {
        e = (AlarmListEntry*)l->data;
        alarm_list_entry_free(e);
    }

    g_list_free(*list);

    *list = NULL;
}
