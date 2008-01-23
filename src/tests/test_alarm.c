#include "alarm.h"
#include "testutils.h"
#include <time.h>
#include <glib.h>
#include <string.h>

#define DUMP_ALARM(alarm)	g_object_get ((alarm), 				\
										  "gconf-dir", &gconf_dir, 	\
										  "id", &id,			\
										  "type", &type,		\
										  "time", &time,		\
										  "message", &message,	\
										  "timer", &timer,		\
										  "notify_type", &ntype,\
										  "sound_file", &sound_file,\
										  "sound_repeat", &sound_repeat,\
										  "command", &command,	\
										  "active", &active,	\
										  NULL);				\
							g_print ("\nAlarm %p:\n"			\
									 "\tgconf-dir: %s\n"		\
									 "\tid: %d\n"				\
									 "\ttype: %s\n"				\
									 "\ttime: %d\n"				\
									 "\tmessage: %s\n"			\
									 "\ttimer: %d\n"			\
									 "\tnotify_type: %s\n"		\
									 "\tsound_file: %s\n"		\
									 "\tsound_repeat: %d\n"		\
									 "\tcommand: %s\n"			\
									 "\tactive: %s\n"			\
									 ,(alarm), gconf_dir, id,	\
									 alarm_type_to_string (type),\
									 time, message, timer,		\
									 alarm_notify_type_to_string (ntype),\
									 sound_file, sound_repeat,	\
									 command, (active) ? "YES" : "NO");

										  
void test_alarm_type (void)
{
	g_print ("TEST ALARM TYPE:\n"
			 "================\n");
	
	g_print ("alarm_type_to_string:\n"
			 "\tALARM_TYPE_INVALID: %s\n"
			 "\tALARM_TYPE_CLOCK: %s\n"
			 "\tALARM_TYPE_TIMER: %s\n",
			 alarm_type_to_string (ALARM_TYPE_INVALID),
			 alarm_type_to_string (ALARM_TYPE_CLOCK),
			 alarm_type_to_string (ALARM_TYPE_TIMER));
	
	g_assert(alarm_type_to_string (ALARM_TYPE_INVALID) == NULL);
	g_assert(strcmp (alarm_type_to_string (ALARM_TYPE_CLOCK), "clock") == 0);
	g_assert(strcmp (alarm_type_to_string (ALARM_TYPE_TIMER), "timer") == 0);
	
	g_print ("alarm_type_from_string:\n"
			 "\tinvalid: %d\n"
			 "\tclock: %d\n"
			 "\ttimer: %d\n",
			 alarm_type_from_string ("invalid"),
			 alarm_type_from_string ("clock"),
			 alarm_type_from_string ("timer"));
	
	g_assert (alarm_type_from_string ("invalid") == ALARM_TYPE_INVALID);
	g_assert (alarm_type_from_string ("clock") == ALARM_TYPE_CLOCK);
	g_assert (alarm_type_from_string ("timer") == ALARM_TYPE_TIMER);
}

void test_alarm_notify_type (void)
{
	g_print ("\nTEST ALARM NOTIFY TYPE:\n"
			 "======================\n");
	
	g_print ("alarm_notify_type_to_string:\n"
			 "\tALARM_NOTIFY_INVALID: %s\n"
			 "\tALARM_NOTIFY_SOUND: %s\n"
			 "\tALARM_NOTIFY_COMMAND: %s\n",
			 alarm_notify_type_to_string (ALARM_NOTIFY_INVALID),
			 alarm_notify_type_to_string (ALARM_NOTIFY_SOUND),
			 alarm_notify_type_to_string (ALARM_NOTIFY_COMMAND));
	
	g_assert(alarm_notify_type_to_string (ALARM_NOTIFY_INVALID) == NULL);
	g_assert(strcmp (alarm_notify_type_to_string (ALARM_NOTIFY_SOUND), "sound") == 0);
	g_assert(strcmp (alarm_notify_type_to_string (ALARM_NOTIFY_COMMAND), "command") == 0);
	
	g_print ("alarm_notify_type_from_string:\n"
			 "\tinvalid: %d\n"
			 "\tsound: %d\n"
			 "\tcommand: %d\n",
			 alarm_notify_type_from_string ("invalid"),
			 alarm_notify_type_from_string ("sound"),
			 alarm_notify_type_from_string ("command"));
	
	g_assert (alarm_notify_type_from_string ("invalid") == ALARM_NOTIFY_INVALID);
	g_assert (alarm_notify_type_from_string ("sound") == ALARM_NOTIFY_SOUND);
	g_assert (alarm_notify_type_from_string ("command") == ALARM_NOTIFY_COMMAND);
}

Alarm *alarm, *alarm2;

void 
test_alarm_object_changed (GObject *object, 
						   GParamSpec *param,
						   gpointer data)
{
	g_print ("Alarm Object %p changed: %s\n", object, g_param_spec_get_name (param));
}

void
test_alarm_object_sound_repeat_changed (GObject *object, 
									    GParamSpec *param,
									    gpointer data)
{
	g_print ("Alarm Object::sound_repeat changed\n");
}

void test_alarm_object (void)
{
	gchar *gconf_dir;
	guint id, time, timer;
	AlarmType type;
	gchar *message;
	AlarmNotifyType ntype;
	gchar *sound_file;
	gboolean sound_repeat;
	gchar *command;
	gboolean active;
	
	g_type_init();
	
	g_print ("\nTEST ALARM OBJECT:\n"
			 "==================\n");
	
	alarm = g_object_new (TYPE_ALARM, 
						  "gconf-dir", "/apps/alarm-applet",
						  NULL);
	
	DUMP_ALARM (alarm);
	
//	g_object_set (alarm, "gconf-dir", "bar", NULL);		// Shouldn't work
	g_object_set (alarm, "id", 0, NULL);
	g_object_set (alarm, "type", ALARM_TYPE_TIMER, NULL);
	g_object_set (alarm, "time", 1234, NULL);
	g_object_set (alarm, "message", "Wakety zooom!", NULL);
	g_object_set (alarm, "timer", 4567, NULL);
	g_object_set (alarm, "notify_type", ALARM_NOTIFY_COMMAND, NULL);
	g_object_set (alarm, "sound_file", "file:///foo/bar", NULL);
	g_object_set (alarm, "sound_repeat", FALSE, NULL);
	g_object_set (alarm, "command", "wiggle-your-toe --arg", NULL);
	
	/* Verify properties */
	ASSERT_PROP_STR_EQUALS	(alarm, "gconf-dir", "/apps/alarm-applet");
	ASSERT_PROP_EQUALS		(alarm, "id", 0, gint);
	ASSERT_PROP_EQUALS		(alarm, "type", ALARM_TYPE_TIMER, gint);
	ASSERT_PROP_EQUALS		(alarm, "time", 1234, gint);
	ASSERT_PROP_STR_EQUALS	(alarm, "message", "Wakety zooom!");
	ASSERT_PROP_EQUALS		(alarm, "timer", 4567, gint);
	ASSERT_PROP_EQUALS		(alarm, "notify_type", ALARM_NOTIFY_COMMAND, gint);
	ASSERT_PROP_STR_EQUALS	(alarm, "sound_file", "file:///foo/bar");
	ASSERT_PROP_EQUALS		(alarm, "sound_repeat", FALSE, gboolean);
	ASSERT_PROP_STR_EQUALS	(alarm, "command", "wiggle-your-toe --arg");
	
	DUMP_ALARM (alarm);
	
	g_object_unref (alarm);
	
	alarm = alarm_new ("/apps/alarm-applet", 0);
	alarm2 = alarm_new ("/apps/alarm-applet", 7);
	
	/* Verify properties */
	ASSERT_PROP_STR_EQUALS	(alarm, "gconf-dir", "/apps/alarm-applet");
	ASSERT_PROP_EQUALS		(alarm, "id", 0, gint);
	ASSERT_PROP_EQUALS		(alarm, "type", ALARM_TYPE_TIMER, gint);
	ASSERT_PROP_EQUALS		(alarm, "time", 1234, gint);
	ASSERT_PROP_STR_EQUALS	(alarm, "message", "Wakety zooom!");
	ASSERT_PROP_EQUALS		(alarm, "timer", 4567, gint);
	ASSERT_PROP_EQUALS		(alarm, "notify_type", ALARM_NOTIFY_COMMAND, gint);
	ASSERT_PROP_STR_EQUALS	(alarm, "sound_file", "file:///foo/bar");
	ASSERT_PROP_EQUALS		(alarm, "sound_repeat", FALSE, gboolean);
	ASSERT_PROP_STR_EQUALS	(alarm, "command", "wiggle-your-toe --arg");
	
	DUMP_ALARM (alarm);
	DUMP_ALARM (alarm2);
	
	g_signal_connect (alarm, "notify", 
					  G_CALLBACK (test_alarm_object_changed),
					  NULL);
	
	g_signal_connect (alarm, "notify::sound-repeat",
					  G_CALLBACK (test_alarm_object_sound_repeat_changed),
					  NULL);
	
	g_signal_connect (alarm2, "notify", 
					  G_CALLBACK (test_alarm_object_changed),
					  NULL);
	
	g_print ("\nBINDING alarm%d/command to alarm%d/message...\n", alarm->id, alarm2->id);
	alarm_bind (alarm, "command", G_OBJECT (alarm2), "message");
}

gboolean
test_alarm_set (gpointer data)
{
	Alarm *alarm = ALARM (data);
	
	g_print ("set sound_repeat\n");
	g_object_set (alarm, "sound_repeat", TRUE, NULL);
}

void
test_alarm_list (void)
{
	GList *list = NULL, *l;
	guint i = 0;
	Alarm *a;
	
	gchar *gconf_dir;
	guint id, time, timer;
	AlarmType type;
	gchar *message;
	AlarmNotifyType ntype;
	gchar *sound_file;
	gboolean sound_repeat;
	gchar *command;
	gboolean active;
	
	g_print ("\nTEST ALARM LIST:\n"
			 "==================\n");
	
	list = alarm_get_list ("/apps/alarm-applet");
	
	for (l = list; l; l = l->next, i++) {
		g_print ("List entry #%d:", i);
		a = ALARM (l->data);
		DUMP_ALARM (a);
		g_object_unref (a);
	}
}

void
test_alarm_signal_alarm (Alarm *a, gchar *data)
{
	g_print ("ALARM on %p! Data: %s\n", a, data);
}

void
test_alarm_signal_error (Alarm *a, GError *err, gchar *data)
{
	g_print ("ERROR on %p! Message: %s, Code: %d, Data: %s", a, err->message, err->code, data);
}

void
test_alarm_signals (void)
{
	g_print ("\nTEST ALARM SIGNALS:\n"
			 "==================\n");
	
	/* Test alarm signals */
	g_signal_connect (alarm, "alarm",
					  G_CALLBACK (test_alarm_signal_alarm),
					  "the data");
	
	alarm_trigger (alarm);
	
	/* Test error signals */
	g_signal_connect (alarm, "error", G_CALLBACK (test_alarm_signal_error),
					  "the error data");
	
	alarm_error_trigger (alarm, 123, "Something bad happened");
}

void
test_alarm_timers (void)
{
	/* Test alarm */
	gint now = time(NULL);
	
	/*g_object_unref (alarm);
	g_object_unref (alarm2);
	
	alarm = alarm_new ("/apps/alarm-applet", 0);
	alarm2 = alarm_new ("/apps/alarm-applet", 1);*/
	
	g_print ("\nTEST ALARM TIMERS:\n"
			 "==================\n");
	
	alarm_disable (alarm);
	alarm_disable (alarm2);
	
	g_print ("test_alarm_timer: Setting alarm #%d (%p) to 5 seconds from now.\n", alarm->id, alarm);
	
	g_object_set (alarm,
				  "type", ALARM_TYPE_CLOCK,
				  "time", now + 5,
				  "active", TRUE,
				  NULL);
	
	g_print ("test_alarm_timer: Setting alarm #%d (%p) TIMER to 10 seconds.\n", alarm2->id, alarm2);

	g_object_set (alarm2, 
				  "type", ALARM_TYPE_TIMER,
				  "timer", 10,
				  NULL);
	
	alarm_enable(alarm2);
}

void
test_alarm_trigger (void)
{
	g_print ("\nTEST ALARM TRIGGER:\n"
			 "==================\n");
	
	g_object_set (alarm,
				  "notify_type", ALARM_NOTIFY_SOUND,
				  "sound_file", "file:///usr/share/sounds/generic.wav",
				  "sound_repeat", FALSE,
				  NULL);
	
	alarm_trigger (alarm);
	
	
	g_object_set (alarm2,
				  "notify_type", ALARM_NOTIFY_COMMAND,
				  "command", "ls -a",
				  NULL);
	
	alarm_trigger (alarm2);
}

gboolean
test_alarm_timer_disable2 (gpointer data)
{
	alarm_disable(alarm2);
	
	return FALSE;
}

int main (void)
{
	GMainLoop *loop;
	
	test_alarm_type ();
	test_alarm_notify_type ();
	test_alarm_object ();
	test_alarm_list ();
	test_alarm_signals ();
	test_alarm_trigger ();
	test_alarm_timers ();
	
	loop = g_main_loop_new (g_main_context_default(), FALSE);
	
//	g_timeout_add_seconds (5, test_alarm_set, alarm);
	
	// Remove alarm from alarm2 after 10 seconds
	//g_timeout_add_seconds (10, test_alarm_timer_disable2, NULL);
	
	g_main_loop_run (loop);
	
	g_object_unref (alarm);
	g_object_unref (alarm2);
	
	return 0;
}
