#include "alarm.h"

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
									 ,(alarm), gconf_dir, id,	\
									 alarm_type_to_string (type),\
									 time, message, 			\
									 alarm_notify_type_to_string (ntype),\
									 sound_file, sound_repeat,	\
									 command);

										  
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

void test_alarm_object (void)
{
	Alarm *alarm;
	
	gchar *gconf_dir;
	guint id, time;
	AlarmType type;
	gchar *message;
	AlarmNotifyType ntype;
	gchar *sound_file;
	gboolean sound_repeat;
	gchar *command;
	
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
}

int main (void)
{
	test_alarm_type ();
	test_alarm_notify_type ();
	test_alarm_object ();
	
	return 0;
}
