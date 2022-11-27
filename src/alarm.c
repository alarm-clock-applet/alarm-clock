// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * alarm.c -- Core alarm functionality
 *
 * Copyright (C) 2007-2008 Johannes H. Jensen <joh@pseudoberries.com>
 * Copyright (C) 2022 Tasos Sahanidis <code@tasossah.com>
 */

#include <stdio.h>
#include <time.h>
#include <string.h>

#include "alarm.h"
#include "alarm-glib-enums.h"
#include <gio/gio.h>

extern void alarm_applet_request_resize(struct _AlarmApplet* applet);

typedef struct _AlarmPrivate AlarmPrivate;

struct _AlarmPrivate {
    GSettings* settings;
    guint gconf_listener;
    guint timer_id;
    MediaPlayer* player;
    guint player_timer_id;
};

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
#endif
G_DEFINE_TYPE_WITH_PRIVATE(Alarm, alarm, G_TYPE_OBJECT);
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

/* Prototypes and constants for property manipulation */
static void alarm_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec);

static void alarm_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec);

static void alarm_dispose(GObject* object);

static void alarm_gsettings_connect(Alarm* alarm);

static void alarm_timer_start(Alarm* alarm);
static void alarm_timer_remove(Alarm* alarm);
static gboolean alarm_timer_is_started(Alarm* alarm);

static void alarm_player_start(Alarm* alarm);
static void alarm_player_stop(Alarm* alarm);
static void alarm_command_run(Alarm* alarm);

#define ALARM_PRIVATE(o) (alarm_get_instance_private(o))

#define ALARM_CAST(o) (G_TYPE_CHECK_INSTANCE_CAST((o), alarm_get_type(), Alarm))

// FIXME: Eventually remove these
typedef struct {
    gint enum_val;
    const gchar* str;
} EnumStringPair;

static EnumStringPair alarm_type_enum_map[] = {
    {ALARM_TYPE_CLOCK,  "clock"},
    { ALARM_TYPE_TIMER, "timer"},
    { 0,                NULL   }
};

static EnumStringPair alarm_repeat_enum_map[] = {
    {ALARM_REPEAT_SUN,  "sun"},
    { ALARM_REPEAT_MON, "mon"},
    { ALARM_REPEAT_TUE, "tue"},
    { ALARM_REPEAT_WED, "wed"},
    { ALARM_REPEAT_THU, "thu"},
    { ALARM_REPEAT_FRI, "fri"},
    { ALARM_REPEAT_SAT, "sat"},
    { 0,                NULL }
};

static EnumStringPair alarm_notify_type_enum_map[] = {
    {ALARM_NOTIFY_SOUND,    "sound"  },
    { ALARM_NOTIFY_COMMAND, "command"},
    { 0,                    NULL     }
};

/* Property indexes */
enum {
    PROP_ALARM_0,
    PROP_ID,
    PROP_TRIGGERED,
    PROP_TYPE,
    PROP_TIME,
    PROP_TIMESTAMP,
    PROP_ACTIVE,
    PROP_MESSAGE,
    PROP_REPEAT,
    PROP_NOTIFY_TYPE,
    PROP_SOUND_FILE,
    PROP_SOUND_LOOP,
    PROP_COMMAND,
};

#define PROP_NAME_ID          "id"
#define PROP_NAME_TRIGGERED   "triggered"
#define PROP_NAME_TYPE        "type"
#define PROP_NAME_TIME        "time"
#define PROP_NAME_TIMESTAMP   "timestamp"
#define PROP_NAME_ACTIVE      "active"
#define PROP_NAME_MESSAGE     "message"
#define PROP_NAME_REPEAT      "repeat"
#define PROP_NAME_NOTIFY_TYPE "notify-type"
#define PROP_NAME_SOUND_FILE  "sound-file"
#define PROP_NAME_SOUND_LOOP  "sound-repeat"
#define PROP_NAME_COMMAND     "command"

/* Signal indexes */
enum {
    SIGNAL_ALARM,
    SIGNAL_CLEARED,
    SIGNAL_ERROR,
    SIGNAL_PLAYER,
    LAST_SIGNAL,
};

/* Signal identifier map */
static guint alarm_signal[LAST_SIGNAL] = { 0, 0, 0, 0 };

/* Prototypes for signal handlers */
static void alarm_alarm(Alarm* alarm);
static void alarm_cleared(Alarm* alarm);
static void alarm_error(Alarm* alarm, GError* err);
static void alarm_player_changed(Alarm* alarm, MediaPlayerState state);

static gboolean alarm_string_to_enum(EnumStringPair lut[], const gchar* str, gint* ret)
{
    for(int i = 0; lut[i].str; i++) {
        if(g_ascii_strcasecmp(lut[i].str, str) == 0) {
            *ret = lut[i].enum_val;
            return TRUE;
        }
    }

    return FALSE;
}

const gchar* alarm_enum_to_string(EnumStringPair lut[], gint enum_val)
{
    for(int i = 0; lut[i].str; i++) {
        if(lut[i].enum_val == enum_val)
            return lut[i].str;
    }

    return NULL;
}


/* Initialize the Alarm class */
static void alarm_class_init(AlarmClass* class)
{
    GParamSpec* id_param;
    GParamSpec* triggered_param;
    GParamSpec* type_param;
    GParamSpec* time_param;
    GParamSpec* timestamp_param;
    GParamSpec* active_param;
    GParamSpec* message_param;
    GParamSpec* repeat_param;
    GParamSpec* notify_type_param;
    GParamSpec* sound_file_param;
    GParamSpec* sound_loop_param;
    GParamSpec* command_param;

    GObjectClass* g_object_class;

    /* get handle to base object */
    g_object_class = G_OBJECT_CLASS(class);

    /* << miscellaneous initialization >> */

    id_param = g_param_spec_uint(PROP_NAME_ID, "alarm id", "id of the alarm", 0, /* min */
                                 UINT_MAX,                                       /* max */
                                 0,                                              /* default */
                                 G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

    triggered_param = g_param_spec_boolean(PROP_NAME_TRIGGERED, "alarm triggered", "triggered flag of the alarm", FALSE, G_PARAM_READWRITE);

    type_param = g_param_spec_enum(PROP_NAME_TYPE, "alarm type", "type of the alarm", ALARM_TYPE_TYPE, ALARM_DEFAULT_TYPE, G_PARAM_READWRITE);


    time_param = g_param_spec_int64(PROP_NAME_TIME, "alarm time", "time when the alarm should trigger", 0, /* min */
                                    G_MAXINT64,                                                            /* max */
                                    ALARM_DEFAULT_TIME,                                                    /* default */
                                    G_PARAM_READWRITE);

    timestamp_param = g_param_spec_int64(PROP_NAME_TIMESTAMP, "alarm timestamp", "UNIX timestamp when the alarm should trigger", 0, /* min */
                                         G_MAXINT64,                                                                                /* max */
                                         ALARM_DEFAULT_TIME,                                                                        /* default */
                                         G_PARAM_READWRITE);

    active_param = g_param_spec_boolean(PROP_NAME_ACTIVE, "active alarm", "whether the alarm is on or not", ALARM_DEFAULT_ACTIVE, G_PARAM_READWRITE);

    message_param = g_param_spec_string(PROP_NAME_MESSAGE, "alarm message", "message which describes this alarm", ALARM_DEFAULT_MESSAGE, G_PARAM_READWRITE);

    repeat_param = g_param_spec_flags(PROP_NAME_REPEAT, "repeat", "repeat the alarm", ALARM_TYPE_REPEAT, ALARM_DEFAULT_REPEAT, /* default */
                                      G_PARAM_READWRITE);

    notify_type_param = g_param_spec_enum(PROP_NAME_NOTIFY_TYPE, "notification type", "what kind of notification should be used", ALARM_TYPE_NOTIFY_TYPE, ALARM_DEFAULT_NOTIFY_TYPE, G_PARAM_READWRITE);

    sound_file_param = g_param_spec_string(PROP_NAME_SOUND_FILE, "sound file", "sound file to play", ALARM_DEFAULT_SOUND_FILE, G_PARAM_READWRITE);

    sound_loop_param = g_param_spec_boolean(PROP_NAME_SOUND_LOOP, "sound loop", "whether the sound should be looped", ALARM_DEFAULT_SOUND_LOOP, G_PARAM_READWRITE);

    command_param = g_param_spec_string(PROP_NAME_COMMAND, "command", "command to run", ALARM_DEFAULT_COMMAND, G_PARAM_READWRITE);

    /* override base object methods */
    g_object_class->set_property = alarm_set_property;
    g_object_class->get_property = alarm_get_property;
    // g_object_class->constructed	 = alarm_constructed;
    g_object_class->dispose = alarm_dispose;

    /* install properties */
    g_object_class_install_property(g_object_class, PROP_ID, id_param);
    g_object_class_install_property(g_object_class, PROP_TRIGGERED, triggered_param);
    g_object_class_install_property(g_object_class, PROP_TYPE, type_param);
    g_object_class_install_property(g_object_class, PROP_TIME, time_param);
    g_object_class_install_property(g_object_class, PROP_TIMESTAMP, timestamp_param);
    g_object_class_install_property(g_object_class, PROP_ACTIVE, active_param);
    g_object_class_install_property(g_object_class, PROP_MESSAGE, message_param);
    g_object_class_install_property(g_object_class, PROP_REPEAT, repeat_param);
    g_object_class_install_property(g_object_class, PROP_NOTIFY_TYPE, notify_type_param);
    g_object_class_install_property(g_object_class, PROP_SOUND_FILE, sound_file_param);
    g_object_class_install_property(g_object_class, PROP_SOUND_LOOP, sound_loop_param);
    g_object_class_install_property(g_object_class, PROP_COMMAND, command_param);

    /* set signal handlers */
    class->alarm = alarm_alarm;
    class->cleared = alarm_cleared;
    class->error = alarm_error;
    class->player_changed = alarm_player_changed;

    /* install signals and default handlers */
    alarm_signal[SIGNAL_ALARM] = g_signal_new("alarm",                            /* name */
                                              TYPE_ALARM,                         /* class type identifier */
                                              G_SIGNAL_RUN_FIRST,                 /* options */
                                              G_STRUCT_OFFSET(AlarmClass, alarm), /* handler offset */
                                              NULL,                               /* accumulator function */
                                              NULL,                               /* accumulator data */
                                              g_cclosure_marshal_VOID__VOID,      /* marshaller */
                                              G_TYPE_NONE,                        /* type of return value */
                                              0);

    alarm_signal[SIGNAL_CLEARED] = g_signal_new("cleared", TYPE_ALARM, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(AlarmClass, cleared), NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

    alarm_signal[SIGNAL_ERROR] = g_signal_new("error", TYPE_ALARM, G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET(AlarmClass, error), NULL, NULL, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);

    alarm_signal[SIGNAL_PLAYER] = g_signal_new("player_changed",                            /* name */
                                               TYPE_ALARM,                                  /* class type identifier */
                                               G_SIGNAL_RUN_LAST,                           /* options */
                                               G_STRUCT_OFFSET(AlarmClass, player_changed), /* handler offset */
                                               NULL,                                        /* accumulator function */
                                               NULL,                                        /* accumulator data */
                                               g_cclosure_marshal_VOID__UINT,               /* marshaller */
                                               G_TYPE_NONE,                                 /* type of return value */
                                               1, G_TYPE_UINT);
}

static void alarm_init(Alarm* self)
{
    AlarmPrivate* priv = ALARM_PRIVATE(self);

    self->id = -1;
}

/* set an Alarm property */
static void alarm_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    AlarmPrivate* priv = ALARM_PRIVATE(ALARM_CAST(object));
    Alarm* alarm = ALARM(object);

    // DEBUGGING INFO
    GValue strval = G_VALUE_INIT;
    g_value_init(&strval, G_TYPE_STRING);
    g_value_transform(value, &strval);
    g_debug("Alarm(%p) #%d: set %s=%s", alarm, alarm->id, pspec->name, g_value_get_string(&strval));
    g_value_unset(&strval);

    alarm->changed = TRUE; // Do this for all properties for now (not too much overhead, anyway)

    switch(prop_id) {
    case PROP_ID:
    {
        // FIXME: This should be int64 to account for -1
        guint d = g_value_get_uint(value);
        if(alarm->id == d)
            break;

        if(priv->settings && alarm->id != -1) {
            g_object_unref(priv->settings);
        }
        alarm->id = d;

        gchar* gsettings_dir = alarm_gsettings_get_dir(alarm);
        priv->settings = g_settings_new_with_path("io.github.alarm-clock-applet.alarm", gsettings_dir);
        g_free(gsettings_dir);

        alarm_gsettings_connect(alarm);
        break;
    }
    case PROP_TRIGGERED:
        // TODO: Should we map this to a GSettings value?
        //       The only case where this might be useful is where the program
        //       is restarted while having a triggered alarm so that we could
        //       re-trigger it when the program starts again...
        alarm->triggered = g_value_get_boolean(value);
        break;
    case PROP_TYPE:
        alarm->type = g_value_get_enum(value);

        if(alarm->active) {
            // Update timestamp
            alarm_update_timestamp(alarm);
        }
        break;
    case PROP_TIME:
        alarm->time = g_value_get_int64(value);

        if(alarm->active) {
            // Update timestamp
            alarm_update_timestamp(alarm);
        }
        break;
    case PROP_TIMESTAMP:
        alarm->timestamp = g_value_get_int64(value);
        break;
    case PROP_ACTIVE:
        alarm->active = g_value_get_boolean(value);

        // g_debug ("[%p] #%d ACTIVE: old=%d new=%d", alarm, alarm->id, b, alarm->active);
        if(alarm->active && !alarm_timer_is_started(alarm)) {
            // Start timer
            alarm_timer_start(alarm);
        } else if(!alarm->active && alarm_timer_is_started(alarm)) {
            // Stop timer
            alarm_timer_remove(alarm);
        }
        break;
    case PROP_MESSAGE:
        g_free(alarm->message);
        alarm->message = g_strdup(g_value_get_string(value));
        break;
    case PROP_REPEAT:
        alarm->repeat = g_value_get_flags(value);

        if(alarm->active)
            alarm_update_timestamp(alarm);

        break;
    case PROP_NOTIFY_TYPE:
        alarm->notify_type = g_value_get_enum(value);
        break;
    case PROP_SOUND_FILE:
        g_free(alarm->sound_file);
        alarm->sound_file = g_strdup(g_value_get_string(value));
        break;
    case PROP_SOUND_LOOP:
        alarm->sound_loop = g_value_get_boolean(value);
        break;
    case PROP_COMMAND:
        g_free(alarm->command);
        alarm->command = g_strdup(g_value_get_string(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

/* retrive an Alarm property */
static void alarm_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
    Alarm* alarm = ALARM(object);

    switch(prop_id) {
    case PROP_ID:
        g_value_set_uint(value, alarm->id);
        break;
    case PROP_TRIGGERED:
        g_value_set_boolean(value, alarm->triggered);
        break;
    case PROP_TYPE:
        g_value_set_enum(value, alarm->type);
        break;
    case PROP_TIME:
        g_value_set_int64(value, alarm->time);
        break;
    case PROP_TIMESTAMP:
        g_value_set_int64(value, alarm->timestamp);
        break;
    case PROP_ACTIVE:
        g_value_set_boolean(value, alarm->active);
        break;
    case PROP_MESSAGE:
        g_value_set_string(value, alarm->message);
        break;
    case PROP_REPEAT:
        g_value_set_flags(value, alarm->repeat);
        break;
    case PROP_NOTIFY_TYPE:
        g_value_set_enum(value, alarm->notify_type);
        break;
    case PROP_SOUND_FILE:
        g_value_set_string(value, alarm->sound_file);
        break;
    case PROP_SOUND_LOOP:
        g_value_set_boolean(value, alarm->sound_loop);
        break;
    case PROP_COMMAND:
        g_value_set_string(value, alarm->command);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

/*
 * ERROR signal {{
 */
GQuark alarm_error_quark(void)
{
    return g_quark_from_static_string("alarm-error-quark");
}

static void alarm_error(Alarm* alarm, GError* err)
{
    g_critical("Alarm(%p) #%d: alarm_error: #%d: %s", alarm, alarm->id, err->code, err->message);
}

void alarm_error_trigger(Alarm* alarm, AlarmErrorCode code, const gchar* msg)
{
    GError* err = g_error_new(ALARM_ERROR, code, "%s", msg);

    g_signal_emit(alarm, alarm_signal[SIGNAL_ERROR], 0, err, NULL);

    g_error_free(err);
    err = NULL;
}

/*
 * }} ERROR signal
 */

static void alarm_player_changed(Alarm* alarm, MediaPlayerState state)
{
    g_debug("Alarm(%p) #%d: player_changed to %d", alarm, alarm->id, state);
}

static inline void alarm_set_triggered(Alarm* alarm, const gboolean triggered)
{
    GValue val = G_VALUE_INIT;
    g_value_init(&val, G_TYPE_BOOLEAN);
    g_value_set_boolean(&val, triggered);
    g_object_set_property(G_OBJECT(alarm), PROP_NAME_TRIGGERED, &val);
    g_value_unset(&val);
}

/*
 * ALARM signal {{
 */

static void alarm_alarm(Alarm* alarm)
{
    g_debug("Alarm(%p) #%d: alarm() DING!", alarm, alarm->id);

    // Clear first, if needed
    alarm_clear(alarm);

    // Update triggered flag
    alarm_set_triggered(alarm, TRUE);

    // Do we want to repeat this alarm?
    if(alarm_should_repeat(alarm)) {
        g_debug("Alarm(%p) #%d: alarm() Repeating...", alarm, alarm->id);
        alarm_update_timestamp(alarm);
    } else {
        alarm_disable(alarm);
    }

    switch(alarm->notify_type) {
    case ALARM_NOTIFY_SOUND:
        // Start sound playback
        g_debug("Alarm(%p) #%d: alarm() Start player", alarm, alarm->id);
        alarm_player_start(alarm);
        break;
    case ALARM_NOTIFY_COMMAND:
        // Start app
        g_debug("Alarm(%p) #%d: alarm() Start command", alarm, alarm->id);
        alarm_command_run(alarm);
        break;
    default:
        g_warning("Alarm(%p) #%d: UNKNOWN NOTIFICATION TYPE %d", alarm, alarm->id, alarm->notify_type);
    }
}

void alarm_trigger(Alarm* alarm)
{
    g_signal_emit(alarm, alarm_signal[SIGNAL_ALARM], 0, NULL);
}

/*
 * Convenience functions for enabling/disabling the alarm.
 *
 * Will update timestamp if needed.
 */
void alarm_set_enabled(Alarm* alarm, gboolean enabled)
{
    if(enabled) {
        alarm_update_timestamp(alarm);
    }

    g_object_set(alarm, "active", enabled, NULL);
}

void alarm_enable(Alarm* alarm)
{
    alarm_set_enabled(alarm, TRUE);
}

void alarm_disable(Alarm* alarm)
{
    alarm_set_enabled(alarm, FALSE);
}

/*
 * Delete alarm. This will remove all configuration
 * associated with this alarm.
 */
void alarm_delete(Alarm* alarm)
{
    AlarmPrivate* priv = ALARM_PRIVATE(alarm);

    g_settings_reset(priv->settings, PROP_NAME_TYPE);
    g_settings_reset(priv->settings, PROP_NAME_TIME);
    g_settings_reset(priv->settings, PROP_NAME_TIMESTAMP);
    g_settings_reset(priv->settings, PROP_NAME_ACTIVE);
    g_settings_reset(priv->settings, PROP_NAME_MESSAGE);
    g_settings_reset(priv->settings, PROP_NAME_REPEAT);
    g_settings_reset(priv->settings, PROP_NAME_NOTIFY_TYPE);
    g_settings_reset(priv->settings, PROP_NAME_SOUND_FILE);
    g_settings_reset(priv->settings, PROP_NAME_SOUND_LOOP);
    g_settings_reset(priv->settings, PROP_NAME_COMMAND);
}

void alarm_unref(Alarm* alarm)
{
    // This should be the final unref call
    g_assert(G_OBJECT(alarm)->ref_count == 1);
    g_object_unref(alarm);
}

/*
 * Snooze the alarm for a number of seconds.
 */
void alarm_snooze(Alarm* alarm, guint seconds)
{
    g_assert(alarm->triggered);

    g_debug("Alarm(%p) #%d: snooze() for %d minutes", alarm, alarm->id, seconds / 60);

    // Silence!
    alarm_clear(alarm);

    // Remind later
    time_t now = time(NULL);

    g_object_set(alarm, "timestamp", now + seconds, "active", TRUE, NULL);

    //    alarm_timer_start (alarm);
}

/**
 * Alarm cleared signal
 */
static void alarm_cleared(Alarm* alarm)
{
    g_debug("Alarm(%p) #%d: cleared()", alarm, alarm->id);

    // Update triggered flag
    alarm_set_triggered(alarm, FALSE);

    // Stop player
    alarm_player_stop(alarm);
}

/*
 * Clear the alarm. Resets the triggered flag and emits the "cleared" signal.
 * This will also stop any running players.
 */
void alarm_clear(Alarm* alarm)
{
    if(alarm->triggered) {
        g_signal_emit(alarm, alarm_signal[SIGNAL_CLEARED], 0, NULL);
    }
}

/*
 * Is the alarm playing?
 */
gboolean alarm_is_playing(Alarm* a)
{
    AlarmPrivate* priv = ALARM_PRIVATE(a);

    return priv->player && priv->player->state == MEDIA_PLAYER_PLAYING;
}


static gboolean alarm_timer_update(Alarm* alarm)
{
    time_t now;

    time(&now);

    if(now >= alarm->timestamp) {
        alarm_trigger(alarm);

        // Remove callback only if we don't intend to repeat the alarm
        return alarm_should_repeat(alarm);
    } else if(alarm->timestamp - now <= 10) {
        g_debug("Alarm(%p) #%d: -%2d...", alarm, alarm->id, (int)(alarm->timestamp - now));
    }

    // Keep callback
    return TRUE;
}

static void alarm_timer_start(Alarm* alarm)
{
    AlarmPrivate* priv = ALARM_PRIVATE(alarm);

    g_debug("Alarm(%p) #%d: timer_start()", alarm, alarm->id);

    // Remove old timer, if any
    alarm_timer_remove(alarm);

    // Keep us updated every 1 second
    priv->timer_id = g_timeout_add_seconds(1, (GSourceFunc)alarm_timer_update, alarm);
}

static gboolean alarm_timer_is_started(Alarm* alarm)
{
    AlarmPrivate* priv = ALARM_PRIVATE(alarm);

    return priv->timer_id > 0;
}

static void alarm_timer_remove(Alarm* alarm)
{
    AlarmPrivate* priv = ALARM_PRIVATE(alarm);

    if(alarm_timer_is_started(alarm)) {
        g_debug("Alarm(%p) #%d: timer_remove", alarm, alarm->id);

        g_source_remove(priv->timer_id);

        priv->timer_id = 0;
    }
}


/*
 * }} ALARM signal
 */

static void alarm_gsettings_connect(Alarm* alarm)
{
    AlarmPrivate* priv = ALARM_PRIVATE(alarm);
    // g_settings_bind(priv->settings, PROP_NAME_TRIGGERED, alarm, PROP_NAME_TRIGGERED, G_SETTINGS_BIND_DEFAULT);
    g_settings_bind(priv->settings, PROP_NAME_TYPE, alarm, PROP_NAME_TYPE, G_SETTINGS_BIND_DEFAULT);
    g_settings_bind(priv->settings, PROP_NAME_TIME, alarm, PROP_NAME_TIME, G_SETTINGS_BIND_DEFAULT);
    g_settings_bind(priv->settings, PROP_NAME_TIMESTAMP, alarm, PROP_NAME_TIMESTAMP, G_SETTINGS_BIND_DEFAULT);
    g_settings_bind(priv->settings, PROP_NAME_ACTIVE, alarm, PROP_NAME_ACTIVE, G_SETTINGS_BIND_DEFAULT);
    g_settings_bind(priv->settings, PROP_NAME_MESSAGE, alarm, PROP_NAME_MESSAGE, G_SETTINGS_BIND_DEFAULT);
    g_settings_bind(priv->settings, PROP_NAME_REPEAT, alarm, PROP_NAME_REPEAT, G_SETTINGS_BIND_DEFAULT);
    g_settings_bind(priv->settings, PROP_NAME_NOTIFY_TYPE, alarm, PROP_NAME_NOTIFY_TYPE, G_SETTINGS_BIND_DEFAULT);
    g_settings_bind(priv->settings, PROP_NAME_SOUND_FILE, alarm, PROP_NAME_SOUND_FILE, G_SETTINGS_BIND_DEFAULT);
    g_settings_bind(priv->settings, PROP_NAME_SOUND_LOOP, alarm, PROP_NAME_SOUND_LOOP, G_SETTINGS_BIND_DEFAULT);
    g_settings_bind(priv->settings, PROP_NAME_COMMAND, alarm, PROP_NAME_COMMAND, G_SETTINGS_BIND_DEFAULT);
}

static void alarm_dispose(GObject* object)
{
    Alarm* alarm = ALARM(object);
    AlarmPrivate* priv = ALARM_PRIVATE(alarm);
    GObjectClass* parent = (GObjectClass*)alarm_parent_class;

    g_debug("Alarm(%p) #%d: dispose()", alarm, alarm->id);

    if(parent->dispose)
        parent->dispose(object);

    g_object_unref(priv->settings);
    alarm_timer_remove(alarm);
    alarm_clear(alarm);
    g_free(alarm->command);
    g_free(alarm->sound_file);
    g_free(alarm->message);
}

// Called every time an alarm is created or deleted
void alarm_update_gsettings_alarm_list(GSettings* settings, GList* alarms)
{
    guint32* newvalues = malloc(((size_t)g_list_length(alarms)) * sizeof(guint32));
    gsize i;
    GList* l;
    for(i = 0, l = alarms; l; i++, l = l->next) {
        const Alarm* a = ALARM(l->data);
        newvalues[i] = a->id;
    }

    GVariant* v = g_variant_new_fixed_array(G_VARIANT_TYPE_UINT32, newvalues, i, sizeof(guint32));
    g_settings_set_value(settings, "alarms", v);
    free(newvalues);
}

static void prop_repeat_notify(GObject* self, GParamSpec* pspec, gpointer user_data)
{
    alarm_applet_request_resize(user_data);
}

/*
 * Convenience function for creating a new alarm instance.
 * Passing -1 as the id will generate a new ID with alarm_gen_id
 */
Alarm* alarm_new(struct _AlarmApplet* applet, GSettings* settings, gint id)
{
    if(id < 0)
        id = alarm_gen_id(settings);

    Alarm* alarm = g_object_new(TYPE_ALARM, "id", id, NULL);

    // Ask for a resize when a property has changed that might require more space
    g_signal_connect(alarm, "notify::" PROP_NAME_REPEAT, G_CALLBACK(prop_repeat_notify), applet);

    return alarm;
}

static inline gboolean in_alarm_array(const guint32* values, const gsize count, const guint32 val)
{
    for(gsize i = 0; i < count; i++)
        if(values[i] == val)
            return TRUE;
    return FALSE;
}

guint alarm_gen_id(GSettings* settings)
{
    GVariant* var = g_settings_get_value(settings, "alarms");
    gsize count = 0;
    const guint32* values = g_variant_get_fixed_array(var, &count, sizeof(guint32));
    guint32 i = 0;
    if(values) {
        for(i = 0; i < G_MAXUINT32; i++)
            if(!in_alarm_array(values, count, i))
                break;
    }
    g_variant_unref(var);

    g_assert(i != G_MAXUINT32);
    return i;
}

gchar* alarm_gsettings_get_dir(Alarm* alarm)
{
    gchar* key;

    g_return_val_if_fail(IS_ALARM(alarm), NULL);

    key = g_strdup_printf(ALARM_G_SETTINGS_BASE_DIR ALARM_G_SETTINGS_DIR_PREFIX "%d/", alarm->id);

    return key;
}

/**
 * Compare two alarms based on ID
 */
static gint alarm_list_item_compare(gconstpointer a, gconstpointer b)
{
    Alarm* a1 = ALARM(a);
    Alarm* a2 = ALARM(b);

    return a1->id - a2->id;
}

/*
 * Get list of alarms in gsettings
 */
GList* alarm_get_list(struct _AlarmApplet* applet, GSettings* settings)
{
    GList* ret = NULL;

    GVariant* var = g_settings_get_value(settings, "alarms");
    gsize count = 0;
    const guint32* values = g_variant_get_fixed_array(var, &count, sizeof(guint32));

    if(values) {
        for(guint32 i = 0; i < count; i++) {
            const guint32 id = values[i];
            g_debug("Alarm: get_list() found #%" G_GUINT32_FORMAT, id);

            Alarm* alarm = alarm_new(applet, settings, id);
            //			g_debug ("\tref = %d", G_OBJECT (alarm)->ref_count);
            ret = g_list_insert_sorted(ret, alarm, alarm_list_item_compare);
            //			g_debug ("\tappend ref = %d", G_OBJECT (alarm)->ref_count);
        }
    }
    g_variant_unref(var);

    return ret;
}

/*
 * Connect a signal callback to all alarms in list.
 */
void alarm_signal_connect_list(GList* instances, const gchar* detailed_signal, GCallback c_handler, gpointer data)
{
    GList* l;
    Alarm* a;
    g_debug("Alarm: signal_connect_list()");
    for(l = instances; l != NULL; l = l->next) {
        a = ALARM(l->data);

        g_debug("\tconnecting Alarm(%p) #%d: %s...", a, a->id, detailed_signal);

        g_signal_connect(a, detailed_signal, c_handler, data);
    }
}


/**
 * Player error
 */
static void alarm_player_error_cb(MediaPlayer* player, GError* err, gpointer data)
{
    Alarm* alarm = ALARM(data);
    gchar* uri;
    gchar* msg;

    uri = media_player_get_uri(player);
    msg = g_strdup_printf("Could not play '%s': %s", uri, err->message);

    g_critical("%s", msg);

    /* Emit error signal */
    alarm_error_trigger(alarm, ALARM_ERROR_PLAY, msg);

    g_free(uri);
    g_free(msg);
}

static void alarm_player_state_cb(MediaPlayer* player, MediaPlayerState state, gpointer data)
{
    static MediaPlayerState prev_state = MEDIA_PLAYER_INVALID;

    Alarm* alarm = ALARM(data);
    AlarmPrivate* priv = ALARM_PRIVATE(alarm);

    if(state != prev_state) {
        // Emit player_changed signal
        g_signal_emit(alarm, alarm_signal[SIGNAL_PLAYER], 0, state, NULL);
    }

    if(state == MEDIA_PLAYER_STOPPED) {
        g_debug("Alarm(%p) #%d: Freeing media player %p", alarm, alarm->id, player);

        media_player_free(player);

        priv->player = NULL;
    }
}

static gboolean alarm_player_timeout(gpointer data)
{
    Alarm* alarm = ALARM(data);

    g_debug("Alarm(%p) #%d: player_timeout", alarm, alarm->id);

    alarm_player_stop(alarm);

    return FALSE;
}


/**
 * Play sound via gstreamer
 */
static void alarm_player_start(Alarm* alarm)
{
    AlarmPrivate* priv = ALARM_PRIVATE(alarm);

    if(priv->player == NULL) {
        priv->player = media_player_new(alarm->sound_file, alarm->sound_loop, alarm_player_state_cb, alarm, alarm_player_error_cb, alarm);
        if(priv->player == NULL) {
            // Unable to create player
            alarm_error_trigger(alarm, ALARM_ERROR_PLAY, _("Could not create player! Please check your sound settings."));
            return;
        }
    } else {
        media_player_set_uri(priv->player, alarm->sound_file);
    }

    media_player_start(priv->player);

    g_debug("Alarm(%p) #%d: player_start...", alarm, alarm->id);

    /*
     * Add stop timeout
     */
    priv->player_timer_id = g_timeout_add_seconds(ALARM_SOUND_TIMEOUT, alarm_player_timeout, alarm);
}

/**
 * Stop player
 */
static void alarm_player_stop(Alarm* alarm)
{
    AlarmPrivate* priv = ALARM_PRIVATE(alarm);

    if(priv->player != NULL) {
        media_player_stop(priv->player);

        if(priv->player_timer_id > 0) {
            g_source_remove(priv->player_timer_id);
            priv->player_timer_id = 0;
        }
    }
}

/*
 * Run Command
 */
static void alarm_command_run(Alarm* alarm)
{
    GError* err = NULL;
    gchar* msg;

    if(!g_spawn_command_line_async(alarm->command, &err)) {

        msg = g_strdup_printf("Could not launch `%s': %s", alarm->command, err->message);

        g_critical("%s", msg);

        /* Emit error signal */
        alarm_error_trigger(alarm, ALARM_ERROR_COMMAND, msg);

        g_free(msg);
        g_error_free(err);
    }
}


/*
 * Set time according to hour, min, sec
 */
void alarm_set_time(Alarm* alarm, guint hour, guint minute, guint second)
{
    g_debug("Alarm(%p) #%d: set_time (%d:%d:%d)", alarm, alarm->id, hour, minute, second);

    g_object_set(alarm, "time", second + minute * 60 + hour * 60 * 60, NULL);
}

/*
 * Calculates the distance from wday1 to wday2
 */
gint alarm_wday_distance(gint wday1, gint wday2)
{
    gint d;

    d = wday2 - wday1;
    if(d < 0)
        d += 7;

    return d;
}

static gboolean alarm_time_is_future(struct tm* tm, guint hour, guint minute, guint second)
{
    return (hour > tm->tm_hour || (hour == tm->tm_hour && minute > tm->tm_min) || (hour == tm->tm_hour && minute == tm->tm_min && second > tm->tm_sec));
}


/*
 * Set time according to hour, min, sec and alarm->repeat
 */
static void alarm_set_timestamp(Alarm* alarm, guint hour, guint minute, guint second)
{
    time_t now, new;
    gint i, d, wday;
    AlarmRepeat rep;
    struct tm tm;

    g_debug("Alarm(%p) #%d: set_timestamp (%d, %d, %d)", alarm, alarm->id, hour, minute, second);

    time(&now);
    tzset();
    if(!localtime_r(&now, &tm)) {
        memset(&tm, 0, sizeof(tm));
        g_critical("Alarm #%d: localtime failed", alarm->id);
    }

    // Automatically detect Daylight Savings Time (DST)
    tm.tm_isdst = -1;

    if(alarm->repeat == ALARM_REPEAT_NONE) {
        // Check if the alarm is for tomorrow
        if(!alarm_time_is_future(&tm, hour, minute, second)) {
            g_debug("\tAlarm is for tomorrow.");
            tm.tm_mday++;
        }
    } else {
        // REPEAT SET: Find the closest repeat day
        wday = -1;

        i = tm.tm_wday;

        // Try finding a day in this week
        for(; i < 7; i++) {
            rep = 1 << i;
            if(alarm->repeat & rep) {
                if(i == tm.tm_wday && !alarm_time_is_future(&tm, hour, minute, second))
                    continue;
                wday = i;
                break;
            }
        }

        // If we haven't found a day in the current week, check next week
        if(wday == -1) {
            for(i = 0; i <= tm.tm_wday; i++) {
                rep = 1 << i;
                if(alarm->repeat & rep) {
                    wday = i;
                    break;
                }
            }
        }

        g_debug("Closest WDAY = %d", wday);

        if(wday == tm.tm_wday && (!alarm_time_is_future(&tm, hour, minute, second)))
            wday = 7;

        // Calculate distance from now to wday
        if(wday == 7) {
            g_debug("\tAlarm is in (forced) 1 week.");
            d = 7;
        } else {
            d = alarm_wday_distance(tm.tm_wday, wday);
        }

        g_debug("\tAlarm is in %d days.", d);

        tm.tm_mday += d;
    }

    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;

    new = mktime(&tm);
    g_debug("\tSetting to %d", (gint) new);
    g_object_set(alarm, "timestamp", new, NULL);
}

/*
 * Update the alarm timestamp to point to the nearest future
 * hour/min/sec according to the time value.
 */
void alarm_update_timestamp(Alarm* alarm)
{
    if(alarm->type == ALARM_TYPE_CLOCK) {
        struct tm tm;
        alarm_get_time(alarm, &tm);
        g_debug("Alarm(%p) #%d: update_timestamp_full: %d:%d:%d", alarm, alarm->id, tm.tm_hour, tm.tm_min, tm.tm_sec);
        alarm_set_timestamp(alarm, tm.tm_hour, tm.tm_min, tm.tm_sec);
    } else {
        /* ALARM_TYPE_TIMER */
        g_object_set(alarm, "timestamp", time(NULL) + alarm->time, NULL);
    }
}

/*
 * Get the alarm time.
 */
void alarm_get_time(Alarm* alarm, struct tm* res)
{
    g_assert(res != NULL);

    if(!gmtime_r(&(alarm->time), res)) {
        memset(res, 0, sizeof(struct tm));
        g_critical("Alarm #%d: gmtime failed", alarm->id);
    }
}

/**
 * Get the remaining alarm time.
 */
void alarm_get_remain(Alarm* alarm, struct tm* res)
{
    g_assert(res != NULL);

    const time_t now = time(NULL);
    res->tm_sec = alarm->timestamp - now;

    res->tm_min = res->tm_sec / 60;
    res->tm_sec -= res->tm_min * 60;

    res->tm_hour = res->tm_min / 60;
    res->tm_min -= res->tm_hour * 60;
}

/**
 * Get the remaining alarm time in seconds
 */
time_t alarm_get_remain_seconds(Alarm* alarm)
{
    time_t now = time(NULL);

    return alarm->timestamp - now;
}


/*
 * AlarmRepeat utilities {{
 */

const gchar* alarm_repeat_to_string(AlarmRepeat repeat)
{
    return alarm_enum_to_string(alarm_repeat_enum_map, repeat);
}

AlarmRepeat alarm_repeat_from_string(const gchar* str)
{
    AlarmRepeat ret = ALARM_REPEAT_NONE;

    if(!str)
        return ret;

    alarm_string_to_enum(alarm_repeat_enum_map, str, (gint*)&ret);

    return ret;
}

AlarmRepeat alarm_repeat_from_list(GSList* list)
{
    GSList* l;
    AlarmRepeat repeat = ALARM_REPEAT_NONE;

    for(l = list; l; l = l->next) {
        repeat |= alarm_repeat_from_string((gchar*)l->data);
    }

    return repeat;
}

GSList* alarm_repeat_to_list(AlarmRepeat repeat)
{
    GSList* list = NULL;
    AlarmRepeat r;
    gint i;

    for(r = ALARM_REPEAT_SUN, i = 0; r <= ALARM_REPEAT_SAT; r = 1 << ++i) {
        if(repeat & r)
            list = g_slist_append(list, (gpointer)alarm_repeat_to_string(r));
    }

    return list;
}

gboolean alarm_should_repeat(Alarm* alarm)
{
    return alarm->type == ALARM_TYPE_CLOCK && alarm->repeat != ALARM_REPEAT_NONE;
}

/*
 * Create a pretty string representation of AlarmRepeat
 *
 * The return value must be freed afterwards
 */
gchar* alarm_repeat_to_pretty(AlarmRepeat repeat)
{
    gchar* str;

    GString* s;
    struct tm tm;
    gchar tmp[20];
    AlarmRepeat r;
    gint i;

    switch(repeat) {
    case ALARM_REPEAT_NONE:
        str = g_strdup(_("Once"));
        break;
    case ALARM_REPEAT_WEEKDAYS:
        str = g_strdup(_("Weekdays"));
        break;
    case ALARM_REPEAT_WEEKENDS:
        str = g_strdup(_("Weekends"));
        break;
    case ALARM_REPEAT_ALL:
        str = g_strdup(_("Every day"));
        break;
    default:
        // Custom repeat, create a list of weekdays
        s = g_string_new("");

        for(r = ALARM_REPEAT_SUN, i = 0; r <= ALARM_REPEAT_SAT; r = 1 << ++i) {
            if(repeat & r) {
                tm.tm_wday = i;
                strftime(tmp, sizeof(tmp), "%a", &tm);
                g_string_append_printf(s, "%s, ", tmp);
            }
        }

        g_string_truncate(s, s->len - 2);

        str = s->str;

        g_string_free(s, FALSE);
        break;
    }

    return str;
}

/*
 * }} AlarmRepeat utilities
 */
