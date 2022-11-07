// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * alarm-gconf.h -- GConf routines
 *
 * Copyright (C) 2007-2008 Johannes H. Jensen <joh@pseudoberries.com>
 * Copyright (C) 2022 Tasos Sahanidis <code@tasossah.com>
 */

#ifndef ALARM_GCONF_H_
#define ALARM_GCONF_H_

#include <time.h>

#include "alarm-applet.h"

G_BEGIN_DECLS

void alarm_applet_gsettings_init(AlarmApplet* applet);

void alarm_applet_gsettings_load(AlarmApplet* applet);

G_END_DECLS

#endif /*ALARM_GCONF_H_*/
