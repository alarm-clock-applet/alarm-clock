// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * alarm.h -- Core alarm functionality
 *
 * Copyright (C) 2007-2008 Johannes H. Jensen <joh@pseudoberries.com>
 * Copyright (C) 2022 Tasos Sahanidis <code@tasossah.com>
 */

#ifndef ALARM_H_
#define ALARM_H_

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#include "player.h"

G_BEGIN_DECLS

struct _AlarmApplet;

/*
 * Utility macros
 */

#define TYPE_ALARM (alarm_get_type())

#define ALARM(object) (G_TYPE_CHECK_INSTANCE_CAST((object), TYPE_ALARM, Alarm))

#define ALARM_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), TYPE_ALARM, AlarmClass))

#define IS_ALARM(object) (G_TYPE_CHECK_INSTANCE_TYPE((object), TYPE_ALARM))

#define IS_ALARM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), TYPE_ALARM))

#define ALARM_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS((object), TYPE_ALARM, AlarmClass))

/*
 * Structure definitions
 */

#include "alarm-enums.h"

#define ALARM_REPEAT_WEEKDAYS (ALARM_REPEAT_MON | ALARM_REPEAT_TUE | ALARM_REPEAT_WED | ALARM_REPEAT_THU | ALARM_REPEAT_FRI)
#define ALARM_REPEAT_WEEKENDS (ALARM_REPEAT_SAT | ALARM_REPEAT_SUN)
#define ALARM_REPEAT_ALL      (ALARM_REPEAT_WEEKDAYS | ALARM_REPEAT_WEEKENDS)

typedef struct _Alarm Alarm;
typedef struct _AlarmClass AlarmClass;

struct _Alarm {
    GObject parent;

    gint id; /* Alarm ID */

    gboolean triggered; // Whether the alarm has been triggered

    /* GConf mapped values */
    AlarmType type;
    time_t time;      /* Time for alarm */
    time_t timestamp; /* UNIX timestamp (local time) for running alarms */
    gboolean active;
    gchar* message;
    AlarmRepeat repeat;

    AlarmNotifyType notify_type;
    gchar* sound_file;
    gboolean sound_loop;
    gchar* command;

    gboolean changed; // Set to TRUE when a property has been changed to request a UI update
};

struct _AlarmClass {
    GObjectClass parent;

    /* Signals */
    void (*alarm)(Alarm* alarm);                                  // Alarm triggered!
    void (*cleared)(Alarm* alarm);                                // Alarm cleared
    void (*error)(Alarm* alarm, GError* err);                     // An error occured
    void (*player_changed)(Alarm* alarm, MediaPlayerState state); /* Media player state changed */
};

/*
 * Error codes
 */
#define ALARM_ERROR alarm_error_quark()

typedef enum {
    ALARM_ERROR_NONE,
    ALARM_ERROR_PLAY,    /* Error playing sound */
    ALARM_ERROR_COMMAND, /* Error launching command */
} AlarmErrorCode;


/*
 * Failsafe defaults for the GConf-mapped properties for
 * use when the schema isn't found or doesn't provide
 * sensible defaults.
 */
#define ALARM_DEFAULT_TYPE        ALARM_TYPE_CLOCK
#define ALARM_DEFAULT_TIME        0
#define ALARM_DEFAULT_TIMESTAMP   0
#define ALARM_DEFAULT_ACTIVE      FALSE
#define ALARM_DEFAULT_MESSAGE     "Alarm!"
#define ALARM_DEFAULT_REPEAT      ALARM_REPEAT_NONE
#define ALARM_DEFAULT_NOTIFY_TYPE ALARM_NOTIFY_SOUND
#define ALARM_DEFAULT_SOUND_FILE  "" // Should default to first in stock sound list
#define ALARM_DEFAULT_SOUND_LOOP  TRUE
#define ALARM_DEFAULT_COMMAND     "" // Should default to first in app list

/*
 * GConf settings
 */

#define ALARM_G_SETTINGS_DIR_PREFIX "alarm-"
#define ALARM_G_SETTINGS_BASE_DIR   "/io/github/alarm-clock-applet/"

/*
 * Player backoff timeout.
 * We will stop the player automatically after 20 minutes.
 */
#define ALARM_SOUND_TIMEOUT (60 * 20)

/*
 * Function prototypes.
 */

/* used by ALARM_TYPE */
GType alarm_get_type(void);

Alarm* alarm_new(struct _AlarmApplet* applet, GSettings* settings, gint id);

guint alarm_gen_id(GSettings* settings);

gchar* alarm_gsettings_get_dir(Alarm* alarm);

const gchar* alarm_type_to_string(AlarmType type);

AlarmType alarm_type_from_string(const gchar* type);


const gchar* alarm_notify_type_to_string(AlarmNotifyType type);

AlarmNotifyType alarm_notify_type_from_string(const gchar* type);

GList* alarm_get_list(struct _AlarmApplet* applet, GSettings* settings);

void alarm_signal_connect_list(GList* instances, const gchar* detailed_signal, GCallback c_handler, gpointer data);

void alarm_trigger(Alarm* alarm);

void alarm_set_enabled(Alarm* alarm, gboolean enabled);

void alarm_enable(Alarm* alarm);

void alarm_clear(Alarm* alarm);

void alarm_disable(Alarm* alarm);

void alarm_delete(Alarm* alarm);

void alarm_unref(Alarm* alarm);

void alarm_snooze(Alarm* alarm, guint seconds);

gboolean alarm_is_playing(Alarm* alarm);

void alarm_update_gsettings_alarm_list(GSettings* settings, GList* alarms);

void alarm_set_time(Alarm* alarm, guint hour, guint minute, guint second);

void alarm_update_timestamp(Alarm* alarm);

void alarm_update_timestamp_full(Alarm* alarm, gboolean include_today);

GQuark alarm_error_quark(void);

void alarm_error_trigger(Alarm* alarm, AlarmErrorCode code, const gchar* msg);

void alarm_get_time(Alarm* alarm, struct tm* res);

struct tm* alarm_get_remain(Alarm* alarm);

time_t alarm_get_remain_seconds(Alarm* alarm);

const gchar* alarm_repeat_to_string(AlarmRepeat repeat);
AlarmRepeat alarm_repeat_from_string(const gchar* str);
AlarmRepeat alarm_repeat_from_list(GSList* list);
GSList* alarm_repeat_to_list(AlarmRepeat repeat);
gint alarm_wday_distance(gint wday1, gint wday2);
gboolean alarm_should_repeat(Alarm* alarm);
gchar* alarm_repeat_to_pretty(AlarmRepeat repeat);


G_END_DECLS

#endif /*ALARM_H_*/
