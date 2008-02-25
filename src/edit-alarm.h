#ifndef EDITALARM_H_
#define EDITALARM_H_

#include "alarm-applet.h"
#include "alarm.h"

typedef struct _AlarmSettingsDialog {
	
	Alarm *alarm;
	
	GtkWidget *dialog;
	GtkWidget *clock_toggle, *timer_toggle;
	GtkWidget *label_entry;
	GtkWidget *hour_spin, *min_spin, *sec_spin;
	
} AlarmSettingsDialog;

void display_add_alarm_dialog ();

#endif /*EDITALARM_H_*/
