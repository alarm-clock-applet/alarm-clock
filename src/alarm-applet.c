// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * alarm-applet.c -- Alarm Clock applet bootstrap
 *
 * Copyright (C) 2007-2008 Johannes H. Jensen <joh@pseudoberries.com>
 * Copyright (C) 2022 Tasos Sahanidis <code@tasossah.com>
 */

#include <stdlib.h>

#include "alarm-applet.h"

#include "alarm.h"
#include "alarm-settings.h"

/*
 * DEFINTIIONS {{
 */

static const gchar* supported_sound_mime_types[] = {
    "audio",
    "video",
    "application/ogg",
    NULL,
};

/*
 * }} DEFINTIIONS
 */

/*
 * Snooze any triggered alarms.
 *
 * Returns the number of snoozed alarms
 */
guint alarm_applet_alarms_snooze(AlarmApplet* applet)
{
    GList* l;
    Alarm* a;
    guint n_snoozed = 0;

    g_debug("Snoozing alarms...");

    // Loop through alarms and snooze all triggered ones
    for(l = applet->alarms; l; l = l->next) {
        a = ALARM(l->data);

        if(a->triggered) {
            alarm_applet_alarm_snooze(applet, a);
            n_snoozed++;
        }
    }

    // Reset the triggered counter
    applet->n_triggered = 0;

    // Update status icon
    alarm_applet_status_update(applet);

    return n_snoozed;
}

/*
 * Stop any running (read: playing sound) alarms.
 */
guint alarm_applet_alarms_stop(AlarmApplet* applet)
{
    GList* l;
    Alarm* a;
    guint n_stopped = 0;

    g_debug("Stopping alarms...");

    // Loop through alarms and clear all of 'em
    for(l = applet->alarms; l; l = l->next) {
        a = ALARM(l->data);

        if(a->triggered) {
            alarm_clear(a);
            n_stopped++;
        }
    }

    // Reset the triggered counter
    applet->n_triggered = 0;

    // Update status icon
    alarm_applet_status_update(applet);

    return n_stopped;
}

/**
 * Snooze an alarm, according to UI settings
 */
void alarm_applet_alarm_snooze(AlarmApplet* applet, Alarm* alarm)
{
    guint mins = applet->snooze_mins;

    if(alarm->type == ALARM_TYPE_CLOCK) {
        // Clocks always snooze for 9 minutes
        mins = ALARM_STD_SNOOZE;
    }

    g_debug("AlarmApplet: snooze '%s' for %d minutes", alarm->message, mins);

    alarm_snooze(alarm, mins * 60);

    // Update status icon
    alarm_applet_status_update(applet);
}

/**
 * Stop an alarm, keeping UI consistent
 */
void alarm_applet_alarm_stop(AlarmApplet* applet, Alarm* alarm)
{
    g_debug("Stopping alarm #%d...", alarm->id);

    // Stop the alarm
    alarm_clear(alarm);

    // Update status icon
    alarm_applet_status_update(applet);
}


/*
 * Sounds list {{
 */

// Load sounds into list
// TODO: Refactor to use a GHashTable with string hash
void alarm_applet_sounds_load(AlarmApplet* applet)
{
    Alarm* alarm;
    AlarmListEntry* entry;
    GList *l, *l2;
    gboolean found;

    const gchar* const* sysdirs;
    gchar* sounds_dir = NULL;
    gchar* tmp;
    gint i;

    // g_assert (applet->alarms);

    // Free old list
    if(applet->sounds != NULL)
        alarm_list_entry_list_free(&(applet->sounds));

    // Locate gnome sounds
    sysdirs = g_get_system_data_dirs();
    for(i = 0; !sounds_dir && sysdirs[i] != NULL; i++) {
        tmp = g_build_filename(sysdirs[i], "sounds/gnome/default/alerts", NULL);
        if(g_file_test(tmp, G_FILE_TEST_IS_DIR)) {
            // Load stock sounds
            g_debug("AlarmApplet: sounds_load: Found %s!", tmp);
            sounds_dir = g_strdup_printf("file://%s", tmp);
            applet->sounds = alarm_list_entry_list_new(sounds_dir, supported_sound_mime_types);
            g_free(sounds_dir);
        }
        g_free(tmp);
    }

    if(!sounds_dir) {
        g_warning("AlarmApplet: Could not locate sounds!");
    }

    // Load custom sounds from alarms
    for(l = applet->alarms; l != NULL; l = l->next) {
        alarm = ALARM(l->data);
        found = FALSE;
        for(l2 = applet->sounds; l2 != NULL; l2 = l2->next) {
            entry = (AlarmListEntry*)l2->data;
            if(strcmp(alarm->sound_file, entry->data) == 0) {
                // FOUND
                found = TRUE;
                break;
            }
        }

        if(!found) {
            // Add to list
            entry = alarm_list_entry_new_file(alarm->sound_file, NULL, NULL);
            if(entry) {
                applet->sounds = g_list_append(applet->sounds, entry);
            }
        }
    }
}

// Notify callback for changes to an alarm's sound_file
static void alarm_sound_file_changed(GObject* object, GParamSpec* param, gpointer data)
{
    Alarm* alarm = ALARM(object);
    AlarmApplet* applet = (AlarmApplet*)data;

    g_debug("alarm_sound_file_changed: #%d", alarm->id);

    // Reload sounds list
    alarm_applet_sounds_load(applet);
}


/*
 * }} Sounds list
 */


/*
 * Apps list {{
 */

static const char* alarm_applet_app_info_command_common(const AlarmApplet* applet, GAppInfo* app)
{
    g_assert(applet->app_command_map != NULL);
    gchar* app_basename = g_path_get_basename(g_app_info_get_executable(app));
    const char* const app_command_ret = g_hash_table_lookup(applet->app_command_map, app_basename);
    g_free(app_basename);
    app_basename = NULL;
    return app_command_ret;
}

const char* alarm_applet_get_app_info_command(const AlarmApplet* applet, GAppInfo* app)
{
    const char* const app_command_ret = alarm_applet_app_info_command_common(applet, app);
    g_assert(app_command_ret != NULL);
    return app_command_ret;
}

static inline gboolean alarm_applet_app_info_command_exists(const AlarmApplet* applet, GAppInfo* app)
{
    return !!alarm_applet_app_info_command_common(applet, app);
}
#define ALARM_APPLET_GET_APP_INFO_COMMAND_EXISTS(x) alarm_applet_app_info_command_exists(applet, x)

static inline gboolean command_exists_in_system(const gchar* cmd)
{
    int argcp;
    char** argvp;
    if(!g_shell_parse_argv(cmd, &argcp, &argvp, NULL))
        return FALSE;

    /*
        In particular, if command_line is an empty string (or a string containing only whitespace),
        G_SHELL_ERROR_EMPTY_STRING will be returned. It’s guaranteed that argvp will be a non-empty
        array if this function returns successfully.
    */

    gchar* found = g_find_program_in_path(argvp[0]);
    if(!found) {
        g_debug("Could not locate %s in PATH", argvp[0]);
        g_strfreev(argvp);
        return FALSE;
    }

    g_debug("located %s at %s", argvp[0], found);
    g_free(found);
    found = NULL;

    g_strfreev(argvp);
    return TRUE;
}

static const char* const app_list_content_type = "audio/x-vorbis+ogg";
static gint g_app_info_same_executable(GAppInfo* appinfo1, GAppInfo* appinfo2)
{
    const char* const first = g_app_info_get_executable(appinfo1);
    const char* const second = g_app_info_get_executable(appinfo2);
    g_assert(first != NULL);
    g_assert(second != NULL);
    return strcmp(first, second);
}

// Load stock apps into list
void alarm_applet_apps_load(AlarmApplet* applet)
{
    // Free the list if it exists already
    g_list_free_full(g_steal_pointer(&applet->apps), g_object_unref);

    // Get all supported apps
    GList* app_list = g_app_info_get_recommended_for_type(app_list_content_type);

    // Get default app
    GAppInfo* default_audio_app = g_app_info_get_default_for_type(app_list_content_type, FALSE);
    g_debug("Default vorbis player: %p %s %s", default_audio_app, g_app_info_get_display_name(default_audio_app), g_app_info_get_executable(default_audio_app));

    // Move default app to the top
    // First, find it in the list
    GList* default_audio_app_item = g_list_find_custom(app_list, default_audio_app, (GCompareFunc)g_app_info_same_executable);
    g_assert(default_audio_app_item != NULL);

    g_object_unref(g_steal_pointer(&default_audio_app));

    // Remove it
    app_list = g_list_remove_link(app_list, default_audio_app_item);

    // Then, prepend it
    app_list = g_list_concat(default_audio_app_item, app_list);

    // Finally, remove any unknown apps
    for(GList* l = app_list; l;) {
        GAppInfo* cur = G_APP_INFO(l->data);
        const char* const executable = g_app_info_get_executable(cur);

        g_debug("Found %s (%s)", g_app_info_get_display_name(cur), executable);

        const gboolean exists = ALARM_APPLET_GET_APP_INFO_COMMAND_EXISTS(cur);
        const char* app_cmd = NULL;
        if(exists && command_exists_in_system((app_cmd = ALARM_APPLET_GET_APP_INFO_COMMAND(cur)))) {
            l = g_list_next(l);
            continue;
        } else if(exists) {
            g_debug("Removing %s because %s doesn't exist", executable, app_cmd);
        } else {
            g_debug("Removing %s because we don't know how to get it to play something", executable);
        }

        GList* to_remove = l;
        l = g_list_next(l);
        app_list = g_list_delete_link(app_list, to_remove);
    }

    applet->apps = app_list;
}

/*
 * Alarms list {{
 */

// TODO: Refactor to use a GHashTable instead?
void alarm_applet_alarms_load(AlarmApplet* applet)
{
    GList* list = NULL;
    GList* l = NULL;

    if(applet->alarms != NULL) {
        // Free old alarm objects
        for(l = applet->alarms; l != NULL; l = l->next) {
            g_object_unref(ALARM(l->data));
        }

        // Free list
        g_list_free(applet->alarms);
    }

    // Fetch list of alarms and add them
    applet->alarms = NULL;
    list = alarm_get_list(applet, applet->settings_global);

    for(l = list; l != NULL; l = l->next) {
        alarm_applet_alarms_add(applet, ALARM(l->data));
    }
}

void alarm_applet_alarms_add(AlarmApplet* applet, Alarm* alarm)
{
    applet->alarms = g_list_append(applet->alarms, alarm);

    g_signal_connect(alarm, "notify", G_CALLBACK(alarm_applet_alarm_changed), applet);
    g_signal_connect(alarm, "notify::sound-file", G_CALLBACK(alarm_sound_file_changed), applet);

    g_signal_connect(alarm, "alarm", G_CALLBACK(alarm_applet_alarm_triggered), applet);
    g_signal_connect(alarm, "cleared", G_CALLBACK(alarm_applet_alarm_cleared), applet);

    // Update alarm list window model
    if(applet->list_window) {
        alarm_list_window_alarm_add(applet->list_window, alarm);
    }
}

void alarm_applet_alarms_remove_and_delete(AlarmApplet* applet, Alarm* alarm)
{
    AlarmSettingsDialog* sdialog = applet->settings_dialog;

    // If there's a settings dialog open for this alarm, close it.
    if(sdialog->alarm == alarm) {
        alarm_settings_dialog_close(sdialog);
    }

    alarm_delete(alarm);

    // Remove from list
    applet->alarms = g_list_remove(applet->alarms, alarm);

    // Clear list store. This will decrease the refcount of our alarms by 1.
    /*if (applet->list_alarms_store)
        gtk_list_store_clear (applet->list_alarms_store);*/

    g_debug("alarm_applet_alarms_remove (..., %p): refcount = %d", alarm, G_OBJECT(alarm)->ref_count);

    // Remove any signal handlers for this alarm instance.
    g_signal_handlers_disconnect_matched(alarm, 0, 0, 0, NULL, NULL, NULL);

    // Update alarm list window model
    if(applet->list_window) {
        alarm_list_window_alarm_remove(applet->list_window, alarm);
    }

    // Dereference alarm
    alarm_unref(alarm);
}

/*
 * }} Alarms list
 */

// TODO: Is this function needed?
/*void
alarm_applet_destroy (AlarmApplet *applet)
{
    GList *l;
    Alarm *a;
    AlarmSettingsDialog *dialog;

    g_debug ("AlarmApplet DESTROY");

    // TODO: Destroy alarms list
//	if (applet->list_alarms_dialog) {
//		list_alarms_dialog_close (applet);
//	}

    // Destroy preferences dialog
    if (applet->prefs_dialog) {
        gtk_widget_destroy (GTK_WIDGET (applet->prefs_dialog));
    }

    // Loop through all alarms and free like a mad man!
    for (l = applet->alarms; l; l = l->next) {
        a = ALARM (l->data);

        // Check if a dialog is open for this alarm
        //dialog = (AlarmSettingsDialog *)g_hash_table_lookup (applet->edit_alarm_dialogs, (gconstpointer)a->id);

        g_object_unref (a);
    }

    // Remove sounds list
    if (applet->sounds) {
        alarm_list_entry_list_free(&(applet->sounds));
    }

    // Remove apps list
    if (applet->apps) {
        alarm_list_entry_list_free(&(applet->apps));
    }

    if (applet->app_command_map) {
        g_hash_table_destroy (applet->app_command_map);
        applet->app_command_map = NULL;
    }

    // Free GConf dir
    //g_free (applet->gconf_dir);

    // Finally free the AlarmApplet struct itself
    g_free (applet);
}*/

/*
 * INIT {{
 */

static inline void alarm_applet_init_app_command_map(AlarmApplet* applet)
{
    applet->app_command_map = g_hash_table_new(g_str_hash, g_str_equal);

    // `rhythmbox-client --play' doesn't actually start playing unless
    // Rhythmbox is already running. Sounds like a Bug.
    g_hash_table_insert(applet->app_command_map, "rhythmbox", "rhythmbox-client --play");

    g_hash_table_insert(applet->app_command_map, "banshee", "banshee --play");

    // Note that totem should already be open with a file for this to work.
    g_hash_table_insert(applet->app_command_map, "totem", "totem --play");

    g_hash_table_insert(applet->app_command_map, "audacious", "audacious --play");

    // The following require playerctl
    g_hash_table_insert(applet->app_command_map, "vlc", "playerctl -p vlc play");

    g_hash_table_insert(applet->app_command_map, "celluloid", "playerctl -p io.github.celluloid_player.Celluloid play");

    // mpv does not have an MPRIS implementation by default
    g_hash_table_insert(applet->app_command_map, "mpv", "playerctl -p mpv play #Needs mpv-mpris");
}

void alarm_applet_activate(GtkApplication* app, gpointer user_data)
{
    AlarmApplet* applet = user_data;

    // Check if there's another instance running
    GList* gtk_windows = gtk_application_get_windows(app);
    if(gtk_windows) {
        gtk_window_present(GTK_WINDOW(gtk_windows->data));
        return;
    }

#ifdef ENABLE_GCONF_MIGRATION
    // Call the GConf -> GSettings migration utility
    if(!g_spawn_command_line_sync("alarm-clock-applet-gconf-migration", NULL, NULL, NULL, NULL))
        g_spawn_command_line_sync("./gconf-migration/alarm-clock-applet-gconf-migration", NULL, NULL, NULL, NULL);
#endif

    // TODO: Add to gsettings
    applet->snooze_mins = 5;

    // Initialize gsettings
    alarm_applet_gsettings_init(applet);

    // Load alarms
    alarm_applet_alarms_load(applet);

    // Load sounds from alarms
    alarm_applet_sounds_load(applet);

    // Initialise map for app commands
    alarm_applet_init_app_command_map(applet);

    // Load apps for alarms
    alarm_applet_apps_load(applet);

    // Set up applet UI
    alarm_applet_ui_init(applet);

    // Show alarms window, unless --hidden
    if(!applet->hidden)
        g_action_activate(G_ACTION(applet->action_toggle_list_win), NULL);
}

/**
 * Cleanup
 */
static void alarm_applet_quit(AlarmApplet* applet)
{
    g_debug("AlarmApplet: Quitting...");
}

static gint handle_local_options(GApplication* application, GVariantDict* options, gpointer user_data)
{
    guint32 count;
    if(g_variant_dict_lookup(options, "version", "b", &count)) {
        g_print(PACKAGE_NAME " " VERSION "\n");
        return 0;
    }

    return -1;
}

static gint handle_command_line(GApplication* application, GApplicationCommandLine* cmdline, gpointer user_data)
{
    AlarmApplet* applet = user_data;
    gboolean stop_all = FALSE;
    gboolean snooze_all = FALSE;

    GVariantDict* options = g_application_command_line_get_options_dict(cmdline);

    if(g_variant_dict_lookup(options, "stop-all", "b", &stop_all))
        g_action_activate(G_ACTION(applet->action_stop_all), NULL);

    if(g_variant_dict_lookup(options, "snooze-all", "b", &snooze_all))
        g_action_activate(G_ACTION(applet->action_snooze_all), NULL);

    if(!(stop_all || snooze_all))
        g_application_activate(G_APPLICATION(application));

    return 0;
}

/**
 * Alarm Clock main()
 */
int main(int argc, char* argv[])
{
    AlarmApplet* applet = NULL;

    // Internationalization
    bindtextdomain(GETTEXT_PACKAGE, ALARM_CLOCK_DATADIR "/locale");
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    // Terminate on critical errors
    // g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL);

    // Initialize GTK+
    GtkApplication* application = gtk_application_new("io.github.alarm-clock-applet", G_APPLICATION_HANDLES_COMMAND_LINE);

    // Initialize applet struct
    applet = g_new0(AlarmApplet, 1);
    applet->application = application;

    g_signal_connect(application, "activate", G_CALLBACK(alarm_applet_activate), applet);

    // Command line options
    applet->hidden = FALSE; // Start hidden

    GOptionEntry entries[] = {
        { "hidden", 'h', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &applet->hidden, _("Start hidden"), NULL },
        { "stop-all", 's', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, NULL, _("Stop all alarms"), NULL },
        { "snooze-all", 'z', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, NULL, _("Snooze all alarms"), NULL },
        { "version", 'v', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, NULL, _("Display version information"), NULL },
        { NULL }
    };
    g_application_add_main_option_entries(G_APPLICATION(application), entries);

    g_signal_connect(application, "handle-local-options", G_CALLBACK(handle_local_options), NULL);
    g_signal_connect(application, "command-line", G_CALLBACK(handle_command_line), applet);

    // Run the main loop
    gint ret = g_application_run(G_APPLICATION(application), argc, argv);

    // Clean up
    // FIXME: check for null?
    alarm_applet_quit(applet);
    g_object_unref(application);

    return ret;
}

/*
 * }} INIT
 */

void alarm_applet_request_resize(AlarmApplet* applet)
{
    if(!applet->list_window)
        g_error("list_window is NULL in alarm_applet_request_resize()");
    alarm_list_request_resize(applet->list_window);
}
