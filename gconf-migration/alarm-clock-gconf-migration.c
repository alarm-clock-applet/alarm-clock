// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * alarm-clock-gconf-migration.c -- GConf to GSettings migration
 *
 * Copyright (C) 2022 Tasos Sahanidis <code@tasossah.com>
 */

#include <glib.h>
#include <gio/gio.h>
#include <gconf/gconf-client.h>
#include <stdio.h>
#include <string.h>

#define MIGRATED_KEY         "gconf-migrated"
#define ALARM_GCONF_DIR      "/apps/alarm-clock"
#define GSETTINGS_ALARM_BASE "/io/github/alarm-clock-applet/"


// Because gsettings-data-convert is absolutely broken on Ubuntu (at least 20.04)...
int main(int argc, char** argv)
{
    int force = (argc == 2 && !strcmp(argv[1], "--force"));

    GSettings* settings = g_settings_new("io.github.alarm-clock-applet");

    if(!force) {
        // First, check if we have finished the migration
        GSettingsSchema* schema = NULL;
        g_object_get(settings, "settings-schema", &schema, NULL);

        // Maybe the key in the schema will be removed at some point in the future
        // So let's assume that if we can't find the key, or if it's true, then the migration has been completed
        gboolean migrated = !g_settings_schema_has_key(schema, MIGRATED_KEY) || g_settings_get_boolean(settings, MIGRATED_KEY);
        g_settings_schema_unref(schema);

        if(migrated) {
            g_object_unref(settings);
            return 0;
        }
    }

    g_print("Migrating config to GSettings...\n");

    // Set up GConf
    GConfClient* gconf = gconf_client_get_default();

    // Copy over the single setting (if it exists)
    GConfValue* val = gconf_client_get_without_default(gconf, ALARM_GCONF_DIR "/show_label", NULL);
    if(val) {
        g_print(ALARM_GCONF_DIR "/show_label -> " GSETTINGS_ALARM_BASE "show_label\n");
        g_settings_set_boolean(settings, "show-label", gconf_value_get_bool(val));
        gconf_value_free(val);
    }

    // Move on to the alarms themselves
    GSList* dlist = gconf_client_all_dirs(gconf, ALARM_GCONF_DIR, NULL);
    GSList* l = NULL;
    if(dlist) {
        // Allocate memory for, and store the alarm ids
        guint32* alarm_ids = malloc(((size_t)g_slist_length(dlist)) * sizeof(guint32));
        guint i = 0;
        // For each alarm
        for(l = dlist; l; l = l->next) {
            gchar* path = l->data;
            guint32 id;
            if(sscanf(path, ALARM_GCONF_DIR "/alarm%" G_GUINT32_FORMAT, &id) != 1) {
                g_print("Unexpected string %s\n", path);
                g_free(path);
                continue;
            }

            alarm_ids[i++] = id;

            gchar alarm_gs_path[128];
            snprintf(alarm_gs_path, sizeof(alarm_gs_path), "%salarm-%" G_GUINT32_FORMAT "/", GSETTINGS_ALARM_BASE, id);
            g_print("%s -> %s\n", path, alarm_gs_path);

            // Make a new object
            GSettings* gs_alarm = g_settings_new_with_path("io.github.alarm-clock-applet.alarm", alarm_gs_path);

            // Get the values from GConf and save them in GSettings
            gchar alarm_gc_path[128];

            // Bools
            snprintf(alarm_gc_path, sizeof(alarm_gc_path), "%s/%s", path, "active");
            g_settings_set_boolean(gs_alarm, "active", gconf_client_get_bool(gconf, alarm_gc_path, NULL));
            snprintf(alarm_gc_path, sizeof(alarm_gc_path), "%s/%s", path, "sound_repeat");
            g_settings_set_boolean(gs_alarm, "sound-repeat", gconf_client_get_bool(gconf, alarm_gc_path, NULL));

            // Strings
            gchar* val;
            snprintf(alarm_gc_path, sizeof(alarm_gc_path), "%s/%s", path, "command");
            val = gconf_client_get_string(gconf, alarm_gc_path, NULL);
            if(val) {
                g_settings_set_string(gs_alarm, "command", val);
                g_free(val);
            }
            snprintf(alarm_gc_path, sizeof(alarm_gc_path), "%s/%s", path, "message");
            val = gconf_client_get_string(gconf, alarm_gc_path, NULL);
            if(val) {
                g_settings_set_string(gs_alarm, "message", val);
                g_free(val);
            }
            snprintf(alarm_gc_path, sizeof(alarm_gc_path), "%s/%s", path, "notify_type");
            val = gconf_client_get_string(gconf, alarm_gc_path, NULL);
            if(val) {
                g_settings_set_string(gs_alarm, "notify-type", val);
                g_free(val);
            }
            snprintf(alarm_gc_path, sizeof(alarm_gc_path), "%s/%s", path, "sound_file");
            val = gconf_client_get_string(gconf, alarm_gc_path, NULL);
            if(val) {
                g_settings_set_string(gs_alarm, "sound-file", val);
                g_free(val);
            }
            snprintf(alarm_gc_path, sizeof(alarm_gc_path), "%s/%s", path, "type");
            val = gconf_client_get_string(gconf, alarm_gc_path, NULL);
            if(val) {
                g_settings_set_string(gs_alarm, "type", val);
                g_free(val);
            }
            val = NULL;

            // 32 bit ints (get converted to 64)
            snprintf(alarm_gc_path, sizeof(alarm_gc_path), "%s/%s", path, "time");
            g_settings_set_int64(gs_alarm, "time", gconf_client_get_int(gconf, alarm_gc_path, NULL));
            snprintf(alarm_gc_path, sizeof(alarm_gc_path), "%s/%s", path, "timestamp");
            g_settings_set_int64(gs_alarm, "timestamp", gconf_client_get_int(gconf, alarm_gc_path, NULL));

            // Finally, the array of strings
            snprintf(alarm_gc_path, sizeof(alarm_gc_path), "%s/%s", path, "repeat");
            GSList* repeat_list = gconf_client_get_list(gconf, alarm_gc_path, GCONF_VALUE_STRING, NULL);
            if(repeat_list) {
                int i = 0;
                // Max 7 days + NULL item at the end
                const gchar* repeat[8] = { NULL };
                for(GSList* ri = repeat_list; ri && i < 7; ri = ri->next, i++) {
                    repeat[i] = ri->data;
                }

                const gchar** ptr = repeat;
                g_settings_set_strv(gs_alarm, "repeat", ptr);

                for(GSList* ri = repeat_list; ri; ri = ri->next) {
                    g_free(ri->data);
                }
                g_slist_free(repeat_list);
            } else {
                g_settings_set_strv(gs_alarm, "repeat", NULL);
            }

            g_free(path);
            path = NULL;

            g_object_unref(gs_alarm);
        }

        // Store the array of alarm ids
        GVariant* v = g_variant_new_fixed_array(G_VARIANT_TYPE_UINT32, alarm_ids, i, sizeof(guint32));
        g_settings_set_value(settings, "alarms", v);

        free(alarm_ids);

        g_slist_free(dlist);
    }

    g_object_unref(gconf);

    g_settings_set_boolean(settings, MIGRATED_KEY, TRUE);

    g_settings_sync();
    g_object_unref(settings);
    g_print("Migration successfully completed!\n");
    return 0;
}
