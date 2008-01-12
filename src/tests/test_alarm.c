#include "alarm.h"
#include <time.h>

#define DUMP_ALARM(alarm)	g_object_get ((alarm), 				\
										  "gconf-dir", &gconf_dir, 	\
										  "id", &id,			\
										  "type", &type,		\
										  "time", &time,		\
										  "message", &message,	\
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
									 "\tnotify_type: %s\n"		\
									 "\tsound_file: %s\n"		\
									 "\tsound_repeat: %d\n"		\
									 "\tcommand: %s\n"			\
									 "\tactive: %s\n"			\
									 ,(alarm), gconf_dir, id,	\
									 alarm_type_to_string (type),\
									 time, message, 			\
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
	
	g_print ("alarm_type_from_string:\n"
			 "\tinvalid: %d\n"
			 "\tclock: %d\n"
			 "\ttimer: %d\n",
			 alarm_type_from_string ("invalid"),
			 alarm_type_from_string ("clock"),
			 alarm_type_from_string ("timer"));
}

void test_alarm_notify_type (void)
{
	g_print ("TEST ALARM NOTIFY TYPE:\n"
			 "======================\n");
	
	g_print ("alarm_notify_type_to_string:\n"
			 "\tALARM_NOTIFY_INVALID: %s\n"
			 "\tALARM_NOTIFY_SOUND: %s\n"
			 "\tALARM_NOTIFY_COMMAND: %s\n",
			 alarm_notify_type_to_string (ALARM_NOTIFY_INVALID),
			 alarm_notify_type_to_string (ALARM_NOTIFY_SOUND),
			 alarm_notify_type_to_string (ALARM_NOTIFY_COMMAND));
	
	g_print ("alarm_notify_type_from_string:\n"
			 "\tinvalid: %d\n"
			 "\tsound: %d\n"
			 "\tcommand: %d\n",
			 alarm_notify_type_from_string ("invalid"),
			 alarm_notify_type_from_string ("sound"),
			 alarm_notify_type_from_string ("command"));
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
	guint id, time;
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
	g_object_set (alarm, "notify_type", ALARM_NOTIFY_COMMAND, NULL);
	g_object_set (alarm, "sound_file", "file:///foo/bar", NULL);
	g_object_set (alarm, "sound_repeat", FALSE, NULL);
	g_object_set (alarm, "command", "wiggle-your-toe --arg", NULL);
	
	DUMP_ALARM (alarm);
	
	g_object_unref (alarm);
	
	alarm = alarm_new ("/apps/alarm-applet", 0);
	alarm2 = alarm_new ("/apps/alarm-applet", 7);
	
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
	
	alarm_bind (alarm, "command", G_OBJECT (alarm2), "message");
}

gboolean
test_alarm_set (gpointer data)
{
	Alarm *alarm = ALARM (data);
	
	g_debug ("set sound_repeat");
	g_object_set (alarm, "sound_repeat", TRUE, NULL);
}

void
test_alarm_list (void)
{
	GList *list = NULL, *l;
	guint i = 0;
	Alarm *a;
	
	gchar *gconf_dir;
	guint id, time;
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
	g_debug ("ALARM on %p! Data: %s", a, data);
}

void
test_alarm_signals (void)
{
	g_print ("\nTEST ALARM SIGNALS:\n"
			 "==================\n");
	
	/* Test signals */
	g_signal_connect (alarm, "alarm",
					  G_CALLBACK (test_alarm_signal_alarm),
					  "the data");
	
	alarm_trigger (alarm);
}

void
test_alarm_timer (void)
{
	/* Test alarm */
	gint now = time(NULL);
	
	/*g_object_unref (alarm);
	g_object_unref (alarm2);
	
	alarm = alarm_new ("/apps/alarm-applet", 0);
	alarm2 = alarm_new ("/apps/alarm-applet", 1);*/
	
	g_print ("\nTEST ALARM TIMER:\n"
			 "==================\n");
	
	
	alarm_disable (alarm);
	alarm_disable (alarm2);
	
	g_print ("test_alarm_timer: Setting alarm (%p) to 10 seconds from now.\n", alarm);
	
	g_object_set (alarm,
				  "time", now + 10,
				  "active", TRUE,
				  NULL);
	
	g_print ("test_alarm_timer: Setting alarm (%p) to 15 seconds from now.\n", alarm2);
	
	g_object_set (alarm2, "time", now + 15, NULL);
	alarm_enable(alarm2);
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
	test_alarm_timer ();
	
	loop = g_main_loop_new (g_main_context_default(), FALSE);
	
//	g_timeout_add_seconds (5, test_alarm_set, alarm);
	
	// Remove alarm from alarm2 after 10 seconds
	g_timeout_add_seconds (10, test_alarm_timer_disable2, NULL);
	
	g_main_loop_run (loop);
	
	g_object_unref (alarm);
	g_object_unref (alarm2);
	
	return 0;
}
