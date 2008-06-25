/*
 * test_alarm.c -- Alarm test suite
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

#include "alarm.h"
#include <time.h>
#include <glib.h>
#include <string.h>

#define GCONF_DIR "/apps/alarm-clock"

/* Fixture */
typedef struct {
	Alarm *alarm;
} AlarmFixture;

static int state = 0;



/*
 * TEST FIXTURE setup
 * 
 * Called before each test case
 */
static void
alarm_fixture_setup (AlarmFixture *fix,
					 gconstpointer test_data)
{
	fix->alarm = alarm_new (GCONF_DIR, 0);
	
	g_assert (fix->alarm != NULL);
}

/*
 * TEST FIXTURE teardown
 * 
 * Called after each test case
 */
static void
alarm_fixture_teardown (AlarmFixture *fix,
						gconstpointer test_data)
{
	g_assert (fix->alarm != NULL);
	
	alarm_delete (fix->alarm);
	g_object_unref (fix->alarm);
}
										  



/*
 * TEST: Alarm properties
 * 
 * 1. Will set properties and verify them
 * 2. Will try to load properties from GConf and verify them
 */
static void
test_alarm_props (AlarmFixture *fix,
				  gconstpointer test_data)
{
	Alarm *alarm = fix->alarm;
	
//	g_object_set (alarm, "gconf-dir", "bar", NULL);		// Shouldn't work
	g_object_set (alarm, 
				  "id", 0,
				  "type", ALARM_TYPE_TIMER,
				  "time", 1234,
				  "timestamp", 5678,
				  "active", FALSE,
				  "message", "Wakety zooom!",
				  "repeat", ALARM_REPEAT_MON | ALARM_REPEAT_WED,
				  "snooze", 12,
				  "notify_type", ALARM_NOTIFY_COMMAND,
				  "sound_file", "file:///foo/bar",
				  "sound_repeat", FALSE, 
				  "command", "wiggle-your-toe --arg",
				  "notify_bubble", TRUE, 
				  NULL);
	
	/* Verify properties */
	g_assert_cmpstr (alarm->gconf_dir, ==, GCONF_DIR);
	g_assert_cmpint (alarm->id, ==, 0);
	g_assert_cmpint (alarm->type, ==, ALARM_TYPE_TIMER);
	g_assert_cmpint (alarm->time, ==, 1234);
	g_assert_cmpint (alarm->timestamp, ==, 5678);
	g_assert_cmpint (alarm->active, ==, FALSE);
	g_assert_cmpstr (alarm->message, ==, "Wakety zooom!");
	g_assert_cmpint (alarm->repeat, ==, ALARM_REPEAT_MON | ALARM_REPEAT_WED);
	g_assert_cmpint (alarm->snooze, ==, 12);
	g_assert_cmpint (alarm->notify_type, ==, ALARM_NOTIFY_COMMAND);
	g_assert_cmpstr (alarm->sound_file, ==, "file:///foo/bar");
	g_assert_cmpint (alarm->sound_loop, ==, FALSE);
	g_assert_cmpstr (alarm->command, ==, "wiggle-your-toe --arg");
	g_assert_cmpint (alarm->notify_bubble, ==, TRUE);
	
	/* Unref alarm */
	g_object_unref (fix->alarm);
	
	/* Load settings from GConf */
	fix->alarm = alarm = alarm_new (GCONF_DIR, 0);
	
	/* Verify properties */
	g_assert_cmpstr (alarm->gconf_dir, ==, GCONF_DIR);
	g_assert_cmpint (alarm->id, ==, 0);
	g_assert_cmpint (alarm->type, ==, ALARM_TYPE_TIMER);
	g_assert_cmpint (alarm->time, ==, 1234);
	g_assert_cmpint (alarm->timestamp, ==, 5678);
	g_assert_cmpint (alarm->active, ==, FALSE);
	g_assert_cmpstr (alarm->message, ==, "Wakety zooom!");
	g_assert_cmpint (alarm->repeat, ==, ALARM_REPEAT_MON | ALARM_REPEAT_WED);
	g_assert_cmpint (alarm->snooze, ==, 12);
	g_assert_cmpint (alarm->notify_type, ==, ALARM_NOTIFY_COMMAND);
	g_assert_cmpstr (alarm->sound_file, ==, "file:///foo/bar");
	g_assert_cmpint (alarm->sound_loop, ==, FALSE);
	g_assert_cmpstr (alarm->command, ==, "wiggle-your-toe --arg");
	g_assert_cmpint (alarm->notify_bubble, ==, TRUE);
}



/*
 * TEST: AlarmType
 */
static void 
test_alarm_type (AlarmFixture *fix,
				 gconstpointer test_data)
{
	// alarm_type_to_string ()
	g_assert        (alarm_type_to_string (ALARM_TYPE_INVALID) ==  NULL);
	g_assert_cmpstr (alarm_type_to_string (ALARM_TYPE_CLOCK),  ==, "clock");
	g_assert_cmpstr (alarm_type_to_string (ALARM_TYPE_TIMER),  ==, "timer");
	
	// alarm_type_from_string ()
	g_assert_cmpint (alarm_type_from_string ("invalid"), ==, ALARM_TYPE_INVALID);
	g_assert_cmpint (alarm_type_from_string ("clock"),   ==, ALARM_TYPE_CLOCK);
	g_assert_cmpint (alarm_type_from_string ("timer"),   ==, ALARM_TYPE_TIMER);
}



/*
 * TEST: AlarmNotifyType
 */
static void 
test_alarm_notify_type (AlarmFixture *fix,
						gconstpointer test_data)
{
	// alarm_notify_type_to_string ()
	g_assert		(alarm_notify_type_to_string (ALARM_NOTIFY_INVALID)  ==  NULL);
	g_assert_cmpstr (alarm_notify_type_to_string (ALARM_NOTIFY_SOUND),   ==, "sound");
	g_assert_cmpstr (alarm_notify_type_to_string (ALARM_NOTIFY_COMMAND), ==, "command");
	
	// alarm_notify_type_from_string ()
	g_assert_cmpint (alarm_notify_type_from_string ("invalid"), ==, ALARM_NOTIFY_INVALID);
	g_assert_cmpint (alarm_notify_type_from_string ("sound"),   ==, ALARM_NOTIFY_SOUND);
	g_assert_cmpint (alarm_notify_type_from_string ("command"), ==, ALARM_NOTIFY_COMMAND);
}



/*
 * TEST: Alarm list
 */
static void
test_alarm_list (AlarmFixture *fix,
				 gconstpointer test_data)
{
	GList *list = NULL, *l;
	guint i = 0;
	Alarm *a, *a1, *a2, *a3;
	
	// Initialize with some dummy alarms
	a = alarm_new (GCONF_DIR, 3);   g_object_unref (a);
	a = alarm_new (GCONF_DIR, 5);   g_object_unref (a);
	a = alarm_new (GCONF_DIR, 8);   g_object_unref (a);
	a = alarm_new (GCONF_DIR, 123); g_object_unref (a);
	
	// Get list
	list = alarm_get_list (GCONF_DIR);
	
	for (l = list; l; l = l->next, i++) {
		int eq;
		switch (i) {
		case 0:
			eq = 3;
			break;
		case 1:
			eq = 5;
			break;
		case 2:
			eq = 8;
			break;
		case 3:
			eq = 123;
			break;
		default:
			g_assert_not_reached ();
			break;
		}
		
		a = ALARM (l->data);
		g_assert_cmpint (a->id, ==, eq);
		
		alarm_delete (a);
		g_object_unref (a);
	}
	
	g_list_free (list);
}



/*
 * Alarm signal handler
 */
static void
test_alarm_signal_alarm (Alarm *a, gchar *data)
{
	//g_print ("ALARM on %p! Data: %s\n", a, data);
	g_assert_cmpstr (data, ==, "the data");
	
	state = 1;
}

/*
 * Error signal handler
 */
static void
test_alarm_signal_error (Alarm *a, GError *err, gchar *data)
{
	//g_print ("ERROR on %p! Message: %s, Code: %d, Data: %s", a, err->message, err->code, data);
	g_assert_cmpint (a->id, ==, 0);
	g_assert_cmpstr (err->message, ==, "Something bad happened");
	g_assert_cmpint (err->code, ==, 123);
	g_assert_cmpstr (data, ==, "the error data");
	
	state = 2;
}



/*
 * TEST: Alarm signals
 * 
 * 1. Test "alarm" signal
 * 2. Test "error" signal
 */
static void
test_alarm_signals (AlarmFixture *fix,
					gconstpointer test_data)
{
	Alarm *alarm = fix->alarm;
	state = 0;
	
	g_object_set (alarm, 
				  "notify_type", ALARM_NOTIFY_COMMAND, 
				  "command", "echo",
				  NULL);
	
	// Test alarm signals
	g_signal_connect (alarm, "alarm",
					  G_CALLBACK (test_alarm_signal_alarm),
					  "the data");
	
	alarm_trigger (alarm);
	
	g_assert_cmpint (state, ==, 1);
	
	
	
	// Test error signals
	g_signal_connect (alarm, "error", G_CALLBACK (test_alarm_signal_error),
					  "the error data");
	
	alarm_error_trigger (alarm, 123, "Something bad happened");
	
	g_assert_cmpint (state, ==, 2);
}



/*
 * Callback function for stopping the GMainLoop
 */
static gboolean
stop_loop (GMainLoop *loop)
{
	g_main_loop_quit (loop);
	return FALSE;
}



/*
 * TEST: Notify types
 * 
 * 1. Test SOUND notification
 * 2. Test COMMAND notification
 */
static void
test_alarm_notify (AlarmFixture *fix,
					gconstpointer test_data)
{
	Alarm *alarm = fix->alarm;
	GMainLoop *loop	= g_main_loop_new (g_main_context_default(), FALSE);
	
	//
	// Test SOUND notification
	//
	g_object_set (alarm,
				  "notify_type", ALARM_NOTIFY_SOUND,
				  "sound_file", "file:///usr/share/sounds/generic.wav",
				  "sound_repeat", FALSE,
				  NULL);
	
	alarm_trigger (alarm);
	
	
	//
	// Test COMMAND notification
	//
	g_object_set (alarm,
				  "notify_type", ALARM_NOTIFY_COMMAND,
				  "command", "ls -a",
				  NULL);
	
	alarm_trigger (alarm);
	
	// We need to run the main loop for a couple of seconds
	// so the player error/state callbacks get the alarm
	// instance while it's still alive.
	g_timeout_add (2000, stop_loop, loop);	// Stop loop after 2 secs
	g_main_loop_run (loop);
}



/*
 * Callback for alarm signal. 
 * Will set state to the current timestamp.
 */
static void
test_alarm_timers_alarm (Alarm *a, gchar *data)
{
	g_debug ("test_alarm_timers_ALARM");
	state = time (NULL);
}



/*
 * TEST: Alarm timers
 * 
 * 1. Test alarm CLOCK setting time to 5 seconds from now.
 * 2. Test alarm TIMER setting time to 3 seconds.
 */
static void
test_alarm_timers (AlarmFixture *fix,
				   gconstpointer test_data)
{
	Alarm *alarm	= fix->alarm;
	time_t now 		= time(NULL);
	struct tm *tm;
	GMainLoop *loop	= g_main_loop_new (g_main_context_default(), FALSE);
	
	
	// Alarm signal handler
	g_signal_connect (alarm, "alarm",
					  G_CALLBACK (test_alarm_timers_alarm),
					  "the data");
	
	// 
	// Test CLOCK 5sec from now
	//
	
	now = time (NULL);
	tm = localtime (&now);
	//g_debug ("TEST SET %d => %d:%d:%d", now, tm->tm_hour, tm->tm_min, tm->tm_sec + 5);
	alarm_set_time (alarm, tm->tm_hour, tm->tm_min, tm->tm_sec + 5);
	
	g_object_set (alarm,
				  "type", ALARM_TYPE_CLOCK,
				  "notify_type", ALARM_NOTIFY_COMMAND,
				  "command", "echo CMDALARM",
				  NULL);
	
	alarm_enable (alarm);
	
	state = 0;
	g_timeout_add (6000, stop_loop, loop);	// Stop loop after 6 secs
	g_main_loop_run (loop);
	
	g_assert_cmpint (state - now, ==, 5);
	
	
	
	// 
	// Test TIMER 3sec
	//
	g_object_set (alarm, 
				  "type", ALARM_TYPE_TIMER,
				  "time", 3,
				  NULL);
	
	alarm_enable (alarm);
	
	state = 0;
	now = time (NULL);
	g_timeout_add (4000, stop_loop, loop);	// Stop loop after 4 secs
	g_main_loop_run (loop);
	
	g_assert_cmpint (state - now, ==, 3);
}



/*
 * TEST: AlarmRepeat
 */
static void
test_alarm_repeat (AlarmFixture *fix,
				   gconstpointer test_data)
{
	AlarmRepeat r, rep;
	const gchar *str;
	gint i, wnow, wday;
	GSList *list, *l;
	struct tm *tm;
	time_t now;
	time (&now);
	
	//
	// Test single repeats to string and back again
	//
	g_print ("SINGLE:\n");
	for (r = ALARM_REPEAT_SUN, i = 1; r <= ALARM_REPEAT_SAT; r = 1 << ++i) {
		str = alarm_repeat_to_string (r);
		rep = alarm_repeat_from_string (str);
		
		g_assert_cmpint (r, ==, rep);
	}
	
	
	// 
	// Test repeat to list
	//
	
	// NONE
	rep = ALARM_REPEAT_NONE;
	list = alarm_repeat_to_list (rep);
	g_assert (NULL == list);
	
	rep = alarm_repeat_from_list (list);
	g_assert_cmpint (rep, ==, ALARM_REPEAT_NONE);
	
	// MULTIPLE
	rep = ALARM_REPEAT_MON | ALARM_REPEAT_WED | ALARM_REPEAT_FRI;
	list = alarm_repeat_to_list (rep);
	
	GString *s = g_string_new ("");
	for (l = list; l; l = l->next) {
		s = g_string_append (s, (gchar *)l->data);
	}
	g_assert_cmpstr (s->str, ==, "monwedfri");
	g_string_free (s, TRUE);
	
	
	r = alarm_repeat_from_list (list);
	g_assert_cmpint (r, ==, rep);
	
	// Should free data as well but we're lazy
	g_slist_free (list);
}

/*
 * TEST: alarm_bind
 */
static void
test_alarm_bind (AlarmFixture *fix,
				 gconstpointer test_data)
{
	Alarm *alarm  = fix->alarm;
	Alarm *alarm2 = alarm_new (GCONF_DIR, 1);
	
	alarm_bind (alarm, "command", G_OBJECT (alarm2), "message");
	
	g_object_set (alarm, "command", "wickid!", NULL);
	
	g_assert_cmpstr (alarm->command, ==, alarm2->message);
	
	alarm_delete (alarm2);
	g_object_unref (alarm2);
}



/*
 * Run them tests!
 */
int main (int argc, char **argv)
{
	g_type_init();
	g_test_init(&argc, &argv, NULL);
	
	// Alarm primitives
	g_test_add ("/alarm/type",		  AlarmFixture, 0, NULL, test_alarm_type, NULL);
	g_test_add ("/alarm/nofity_type", AlarmFixture, 0, NULL, test_alarm_notify_type, NULL);
	g_test_add ("/alarm/list",		  AlarmFixture, 0, NULL, test_alarm_list, NULL);
	g_test_add ("/alarm/repeat",	  AlarmFixture, 0, NULL, test_alarm_repeat, NULL);
	
	// Full system tests
	g_test_add ("/alarm/props",  AlarmFixture, 0, alarm_fixture_setup, test_alarm_props, alarm_fixture_teardown);
	g_test_add ("/alarm/signal", AlarmFixture, 0, alarm_fixture_setup, test_alarm_signals, alarm_fixture_teardown);
	g_test_add ("/alarm/notify", AlarmFixture, 0, alarm_fixture_setup, test_alarm_notify, alarm_fixture_teardown);
	g_test_add ("/alarm/bind",	 AlarmFixture, 0, alarm_fixture_setup, test_alarm_bind, alarm_fixture_teardown);
	g_test_add ("/alarm/timers", AlarmFixture, 0, alarm_fixture_setup, test_alarm_timers, alarm_fixture_teardown);
	
	return g_test_run ();
}
