#ifndef ALARM_GCONF_H_
#define ALARM_GCONF_H_

#include <time.h>
#include <panel-applet-gconf.h>

#include "alarm-applet.h"

G_BEGIN_DECLS

/* Global GConf keys */
#define KEY_SHOW_LABEL		"show_label"
#define KEY_LABEL_TYPE		"label_type"

#define N_GCONF_PREFS		3	/* + global gconf listener */

/* Failsafe defaults for use when the schema isn't found
 * or doesn't provide sensible defaults */
#define DEF_SHOW_LABEL		TRUE
#define DEF_LABEL_TYPE		LABEL_TYPE_TIME

/* We store enums as strings */
GConfEnumStringPair label_type_enum_map [];

/*
 * Init
 */
void
alarm_applet_gconf_init (AlarmApplet *applet);

void
alarm_applet_gconf_load (AlarmApplet *applet);

G_END_DECLS

#endif /*ALARM_GCONF_H_*/
