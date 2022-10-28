/*
 * alarm-gconf.c -- GConf routines
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

#include <time.h>

#include "alarm-applet.h"
#include "alarm-gsettings.h"
#include "alarm-settings.h"
#include "alarm.h"

static inline gboolean alarm_in_alarm_list(guint32 id, GList* alarms)
{
    for(GList* l = alarms; l; l = l->next) {
        if(id == ALARM(l->data)->id)
            return TRUE;
    }
    return FALSE;
}

static inline gboolean alarm_in_gsettings_list(guint32 id, const guint32* values, gsize count)
{
    for(guint32 i = 0; i < count; i++) {
        if(values[i] == id)
            return TRUE;
    }
    return FALSE;
}

void alarm_list_changed(GSettings* self, gchar* key, gpointer user_data)
{
    AlarmApplet* applet = user_data;
    g_debug("alarm_list_changed");

    // Get new list of alarms
    GVariant* var = g_settings_get_value(self, "alarms");
    gsize count = 0;
    const guint32* values = g_variant_get_fixed_array(var, &count, sizeof(guint32));

    // First, check if any new alarms have been added
    for(guint32 i = 0; i < count; i++) {
        const guint32 settings_id = values[i];
        // Add the alarm if it doesn't exist
        if(!alarm_in_alarm_list(settings_id, applet->alarms)) {
            Alarm* a = alarm_new(applet, self, settings_id);

            g_debug("\tADD alarm #%d %p", settings_id, a);

            alarm_applet_alarms_add(applet, a);
        }
    }

    // Finally, check if any existing alarms no longer exist
    GList* l = applet->alarms;
    while(l) {
        Alarm* a = ALARM(l->data);
        if(!alarm_in_gsettings_list(a->id, values, count)) {

            g_debug("\tDELETE alarm #%d %p", a->id, a);

            alarm_disable(a);
            alarm_clear(a);

            // Remove from list
            alarm_applet_alarms_remove_and_delete(applet, a);

            // Start loop from beginning
            l = applet->alarms;
            continue;
        }
        l = l->next;
    }

    g_variant_unref(var);
}

void alarm_show_label_changed(GSettings* self, gchar* key, gpointer user_data)
{
    g_debug ("alarm_show_label_changed");
    prefs_show_label_update(user_data);
}

/*
 * Init
 */
void
alarm_applet_gsettings_init (AlarmApplet *applet)
{
	applet->settings_global = g_settings_new("io.github.alarm-clock-applet");
    g_signal_connect(applet->settings_global, "changed::alarms", G_CALLBACK(alarm_list_changed), applet);
    // Maybe GSettingsAction would work better here. If one can figure out how to use it, that is.
    g_signal_connect(applet->settings_global, "changed::show-label", G_CALLBACK(alarm_show_label_changed), applet);
}
