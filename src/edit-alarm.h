#ifndef EDITALARM_H_
#define EDITALARM_H_

#include "alarm-applet.h"
#include "alarm.h"
#include "player.h"

typedef struct _AlarmSettingsDialog {
	
	AlarmApplet *applet;
	Alarm *alarm;
	
	/* Media player for previews */
	MediaPlayer *player;
	
	GtkWidget *dialog;
	GtkWidget *clock_toggle, *timer_toggle;
	GtkWidget *label_entry;
	GtkWidget *hour_spin, *min_spin, *sec_spin;
	
	/* Notification */
	GtkWidget *notify_sound_radio;
	GtkWidget *notify_sound_box;
	GtkWidget *notify_sound_stock;
	GtkWidget *notify_sound_combo;
	GtkWidget *notify_sound_loop_check;
	GtkWidget *notify_sound_preview;
	
	GtkWidget *notify_app_radio;
	GtkWidget *notify_app_box;
	GtkWidget *notify_app_combo;
	GtkWidget *notify_app_command_box;
	GtkWidget *notify_app_command_entry;
	
	GtkWidget *notify_bubble_check;
	
} AlarmSettingsDialog;

void display_add_alarm_dialog ();

#endif /*EDITALARM_H_*/
