#ifndef ALARM_GCONF_H_
#define ALARM_GCONF_H_

#include <time.h>
#include <panel-applet-gconf.h>

#include "alarm-applet.h"

G_BEGIN_DECLS

/* GConf keys */
#define KEY_ALARMTIME		"alarm_time"
#define KEY_MESSAGE			"message"
#define KEY_STARTED			"started"
#define KEY_SHOW_LABEL		"show_label"
#define KEY_LABEL_TYPE		"label_type"
#define KEY_NOTIFY_TYPE		"notify_type"
#define KEY_SOUND_FILE		"sound_file"
#define KEY_SOUND_LOOP		"sound_repeat"
#define KEY_COMMAND			"command"
#define KEY_NOTIFY_BUBBLE	"notify_bubble"

#define N_GCONF_PREFS	10

GConfEnumStringPair label_type_enum_map [];

GConfEnumStringPair notify_type_enum_map [];

/**
 * Stores the alarm timestamp in gconf
 */
void
alarm_gconf_set_alarm (AlarmApplet *applet, guint hour, guint minute, guint second);

/**
 * Stores the alarm message in gconf
 */
void
alarm_gconf_set_message (AlarmApplet *applet, const gchar *message);

/**
 * Sets started status of alarm in gconf
 */
void
alarm_gconf_set_started (AlarmApplet *applet, gboolean started);



/*
 * Init
 */
void
setup_gconf (AlarmApplet *applet);

G_END_DECLS

#endif /*ALARM_GCONF_H_*/
