#ifndef PREFS_H_
#define PREFS_H_

#include "alarm-applet.h"

void
preferences_dialog_response_cb (GtkDialog *dialog,
								gint rid,
								AlarmApplet *applet);

void
pref_update_label_show (AlarmApplet *applet);

void
pref_update_label_type (AlarmApplet *applet);

void
pref_update_notify_type (AlarmApplet *applet);

void
pref_update_sound_file (AlarmApplet *applet);

void
pref_update_sound_loop (AlarmApplet *applet);

void
pref_update_command (AlarmApplet *applet);

void
pref_update_show_bubble (AlarmApplet *applet);

void
preferences_dialog_display (AlarmApplet *applet);

void
pref_label_show_changed_cb (GtkToggleButton *togglebutton,
							AlarmApplet *applet);

void
pref_label_type_changed_cb (GtkToggleButton *togglebutton,
							AlarmApplet *applet);

void
pref_notify_type_changed_cb (GtkToggleButton *togglebutton,
							AlarmApplet *applet);

void
pref_open_sound_file_chooser (AlarmApplet *applet);

void
pref_notify_sound_combo_changed_cb (GtkComboBox *combo,
									AlarmApplet *applet);

void
pref_notify_sound_loop_changed_cb (GtkToggleButton *togglebutton,
							AlarmApplet *applet);

void
pref_play_sound_cb (GtkButton *button,
					AlarmApplet *applet);

void
pref_notify_app_combo_changed_cb (GtkComboBox *combo,
								  AlarmApplet *applet);

void
pref_notify_app_command_changed_cb (GtkEditable *editable,
								AlarmApplet *applet);

void
pref_notify_bubble_changed_cb (GtkToggleButton *togglebutton,
							AlarmApplet *applet);

void
load_stock_sounds_list (AlarmApplet *applet);

void
load_app_list (AlarmApplet *applet);

#endif /*PREFS_H_*/
