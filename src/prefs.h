// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * prefs.h -- Alarm Clock global preferences
 *
 * Copyright (C) 2007-2008 Johannes H. Jensen <joh@pseudoberries.com>
 * Copyright (C) 2022 Tasos Sahanidis <code@tasossah.com>
 */

#ifndef PREFS_H_
#define PREFS_H_

#include "alarm-applet.h"

void prefs_init(AlarmApplet* applet);

void prefs_dialog_show(AlarmApplet* applet);

gboolean prefs_autostart_get_state(void);

void prefs_autostart_set_state(gboolean state);

gboolean prefs_show_label_get(AlarmApplet* applet);

void prefs_show_label_set(AlarmApplet* applet, gboolean state);

void prefs_show_label_update(AlarmApplet* applet);

#endif /*PREFS_H_*/
