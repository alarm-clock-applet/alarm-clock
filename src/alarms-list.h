#ifndef ALARMSLIST_H_
#define ALARMSLIST_H_

#include "alarm-applet.h"
#include "alarm.h"

typedef enum {
	TYPE_COLUMN,
	ACTIVE_COLUMN,
	TIME_COLUMN,
	LABEL_COLUMN,
	PLAYING_COLUMN,
	ALARMS_N_COLUMNS
} AlarmColumn;


void
list_alarms_dialog_display (AlarmApplet *applet);

void
list_alarms_dialog_close (AlarmApplet *applet);

void
list_alarms_update (AlarmApplet *applet);

#endif /*ALARMSLIST_H_*/
