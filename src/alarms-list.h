#ifndef ALARMSLIST_H_
#define ALARMSLIST_H_

#include "alarm-applet.h"
#include "alarm.h"

enum {
	TYPE_COLUMN,
	ACTIVE_COLUMN,
	TIME_COLUMN,
	LABEL_COLUMN,
	ALARMS_N_COLUMNS
};


void
display_list_alarms_dialog (AlarmApplet *applet);

#endif /*ALARMSLIST_H_*/
