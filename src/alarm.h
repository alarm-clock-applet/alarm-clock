/*
 * alarm.h -- Core alarm functionality
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

#ifndef ALARM_H_
#define ALARM_H_

#include <glib-object.h>
#include <gconf/gconf-client.h>

#include "player.h"

G_BEGIN_DECLS

/*
 * Utility macros
 */

#define TYPE_ALARM (alarm_get_type())

#define ALARM(object) \
  (G_TYPE_CHECK_INSTANCE_CAST((object), TYPE_ALARM, Alarm))

#define ALARM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), TYPE_ALARM, AlarmClass))

#define IS_ALARM(object) \
  (G_TYPE_CHECK_INSTANCE_TYPE((object), TYPE_ALARM))

#define IS_ALARM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), TYPE_ALARM))

#define ALARM_GET_CLASS(object) \
  (G_TYPE_INSTANCE_GET_CLASS((object), TYPE_ALARM, AlarmClass))

/*
 * Structure definitions
 */

typedef enum {
	ALARM_TYPE_INVALID = 0,
	ALARM_TYPE_CLOCK,		/* Alarm at specific time */
	ALARM_TYPE_TIMER		/* Alarm in X mins */
} AlarmType;

typedef enum {
	ALARM_REPEAT_NONE = 0,
	ALARM_REPEAT_SUN  = 1 << 0,
	ALARM_REPEAT_MON  = 1 << 1,
	ALARM_REPEAT_TUE  = 1 << 2,
	ALARM_REPEAT_WED  = 1 << 3,
	ALARM_REPEAT_THU  = 1 << 4,
	ALARM_REPEAT_FRI  = 1 << 5,
	ALARM_REPEAT_SAT  = 1 << 6,
} AlarmRepeat;

#define ALARM_REPEAT_WEEKDAYS	(ALARM_REPEAT_MON | ALARM_REPEAT_TUE | ALARM_REPEAT_WED | ALARM_REPEAT_THU | ALARM_REPEAT_FRI)
#define ALARM_REPEAT_WEEKENDS	(ALARM_REPEAT_SAT | ALARM_REPEAT_SUN)
#define ALARM_REPEAT_ALL		(ALARM_REPEAT_WEEKDAYS | ALARM_REPEAT_WEEKENDS)

typedef enum {
	ALARM_NOTIFY_INVALID = 0,
	ALARM_NOTIFY_SOUND,		/* Notification by sound */
	ALARM_NOTIFY_COMMAND,	/* Notification by command */
} AlarmNotifyType;

typedef struct _Alarm Alarm;
typedef struct _AlarmClass AlarmClass;

struct _Alarm {
	GObject parent;

	gchar *gconf_dir;		/* GConf directory */
	gint id;				/* Alarm ID */

	/* GConf mapped values */
	AlarmType type;
	time_t time;			/* Time for alarm */
	time_t timestamp;		/* UNIX timestamp (local time) for running alarms */
	gboolean active;
	gchar *message;
	AlarmRepeat repeat;
	gint snooze;

	AlarmNotifyType notify_type;
	gchar *sound_file;
	gboolean sound_loop;
	gchar *command;
	gboolean notify_bubble;
};

struct _AlarmClass {
	GObjectClass parent;

	/* Signals */
	void (*alarm)(Alarm *alarm);				/* Alarm triggered! */
	void (*error)(Alarm *alarm, GError *err);	/* An error occured */
	void (*player_changed)(Alarm *alarm, MediaPlayerState state);		/* Media player state changed */
};

/*
 * Error codes
 */
#define ALARM_ERROR		alarm_error_quark ()

typedef enum {
	ALARM_ERROR_NONE,
	ALARM_ERROR_PLAY,		/* Error playing sound */
	ALARM_ERROR_COMMAND		/* Error launching command */
} AlarmErrorCode;


/*
 * Failsafe defaults for the GConf-mapped properties for
 * use when the schema isn't found or doesn't provide
 * sensible defaults.
 */
#define ALARM_DEFAULT_TYPE			ALARM_TYPE_CLOCK
#define ALARM_DEFAULT_TIME			0
#define ALARM_DEFAULT_TIMESTAMP		0
#define ALARM_DEFAULT_ACTIVE		FALSE
#define ALARM_DEFAULT_MESSAGE		"Alarm!"
#define ALARM_DEFAULT_REPEAT		ALARM_REPEAT_NONE
#define ALARM_DEFAULT_SNOOZE		0
#define ALARM_DEFAULT_NOTIFY_TYPE	ALARM_NOTIFY_SOUND
#define ALARM_DEFAULT_SOUND_FILE	""				// Should default to first in stock sound list
#define ALARM_DEFAULT_SOUND_LOOP	TRUE
#define ALARM_DEFAULT_COMMAND		""				// Should default to first in app list
#define ALARM_DEFAULT_NOTIFY_BUBBLE	TRUE

/*
 * GConf settings
 */
#define ALARM_GCONF_DIR_PREFIX		"alarm"
#define ALARM_GCONF_SCHEMA_DIR		"/schemas/apps/alarm_clock/alarm"

/*
 * Player backoff timeout.
 * We will stop the player automatically after 20 minutes.
 */
#define ALARM_SOUND_TIMEOUT			(60 * 20)

/*
 * Function prototypes.
 */

/* used by ALARM_TYPE */
GType
alarm_get_type (void);

Alarm *
alarm_new (const gchar *gconf_dir, gint id);

guint
alarm_gen_id_dir (const gchar *gconf_dir);

guint
alarm_gen_id (Alarm *alarm);

gchar *
alarm_gconf_get_dir (Alarm *alarm);

gint
alarm_gconf_dir_get_id (const gchar *dir);

gchar *
alarm_gconf_get_full_key (Alarm *alarm, const gchar *key);


const gchar *
alarm_type_to_string (AlarmType type);

AlarmType
alarm_type_from_string (const gchar *type);


const gchar *
alarm_notify_type_to_string (AlarmNotifyType type);

AlarmNotifyType
alarm_notify_type_from_string (const gchar *type);

GList *
alarm_get_list (const gchar *gconf_dir);

void
alarm_signal_connect_list (GList *instances,
						   const gchar *detailed_signal,
						   GCallback c_handler,
						   gpointer data);

void
alarm_trigger (Alarm *alarm);

void
alarm_set_enabled (Alarm *alarm, gboolean enabled);

void
alarm_enable (Alarm *alarm);

void
alarm_clear (Alarm *alarm);

void
alarm_disable (Alarm *alarm);

void
alarm_delete (Alarm *alarm);

void
alarm_snooze (Alarm *alarm);

void
alarm_set_time (Alarm *alarm, guint hour, guint minute, guint second);

void
alarm_update_timestamp (Alarm *alarm);

void
alarm_update_timestamp_full (Alarm *alarm, gboolean include_today);

GQuark
alarm_error_quark (void);

void
alarm_error_trigger (Alarm *alarm, AlarmErrorCode code, const gchar *msg);

struct tm *
alarm_get_time (Alarm *alarm);

struct tm *
alarm_get_remain (Alarm *alarm);

const gchar *alarm_repeat_to_string (AlarmRepeat repeat);
AlarmRepeat alarm_repeat_from_string (const gchar *str);
AlarmRepeat alarm_repeat_from_list (GSList *list);
GSList *alarm_repeat_to_list (AlarmRepeat repeat);
gint alarm_wday_distance (gint wday1, gint wday2);
gboolean alarm_should_repeat (Alarm *alarm);

G_END_DECLS

#endif /*ALARM_H_*/
