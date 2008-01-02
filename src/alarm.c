/*
 * Copyright/Licensing information.
 */

#include "alarm.h"

static GConfEnumStringPair alarm_type_enum_map [] = {
	{ ALARM_TYPE_CLOCK,		"clock" },
	{ ALARM_TYPE_TIMER,		"timer" },
	{ 0, NULL }
};

static GConfEnumStringPair alarm_notify_type_enum_map [] = {
	{ ALARM_NOTIFY_SOUND,	"sound"  },
	{ ALARM_NOTIFY_COMMAND,	"command" },
	{ 0, NULL }
};

static enum {
	PROP_ALARM_0,
	PROP_DIR,
	PROP_ID,
	PROP_TYPE,
	PROP_TIME,
	PROP_MESSAGE,
	PROP_NOTIFY_TYPE,
	PROP_SOUND_FILE,
	PROP_SOUND_LOOP,
	PROP_COMMAND
} AlarmProps;

#define PROP_NAME_DIR			"gconf-dir"
#define PROP_NAME_ID			"id"
#define PROP_NAME_TYPE			"type"
#define PROP_NAME_TIME			"time"
#define PROP_NAME_MESSAGE		"message"
#define PROP_NAME_NOTIFY_TYPE	"notify_type"
#define PROP_NAME_SOUND_FILE	"sound_file"
#define PROP_NAME_SOUND_LOOP	"sound_repeat"
#define PROP_NAME_COMMAND		"command"

/* get the identifier */
GType alarm_get_type (void)
{
	static GType type = 0;
	
	if (!type)
	{
		 static const GTypeInfo info = {
				sizeof(AlarmClass),					/* class structure size */
				NULL,								/* base class initializer */
				NULL,								/* base class finalizer */
				(GClassInitFunc) alarm_class_init,	/* class initializer */
				NULL,								/* class finalizer */
				NULL,								/* class data */
				sizeof(Alarm),						/* instance structure size */
				0,									/* preallocated instances */
				NULL,								/* instance initializer */
				NULL								/* function table */
		 };

		type = g_type_register_static( G_TYPE_OBJECT,	/* parent class */
										 "Alarm",			/* type name */
										 &info,			/* GTypeInfo struct (above) */
										 0);				/* flags */
	}

	return type;
}

/* Initialize the Alarm class */
static void 
alarm_class_init (AlarmClass *class)
{
	GParamSpec *dir_param;
	GParamSpec *id_param;
	GParamSpec *type_param;
	GParamSpec *time_param;
	GParamSpec *message_param;
	GParamSpec *notify_type_param;
	GParamSpec *sound_file_param;
	GParamSpec *sound_loop_param;
	GParamSpec *command_param;
	
	GObjectClass *g_object_class;

	/* get handle to base object */
	g_object_class = G_OBJECT_CLASS (class);

	/* << miscellaneous initialization >> */

	/* create GParamSpec descriptions for properties */
	dir_param = g_param_spec_string (PROP_NAME_DIR,
									 "GConf dir",
									 "GConf base directory",
									 NULL,
									 G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
	
	id_param = g_param_spec_uint (PROP_NAME_ID,
								  "alarm id",
								  "id of the alarm",
								  0,		/* min */
								  UINT_MAX,	/* max */
								  0,		/* default */
								  G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
	
	type_param = g_param_spec_uint (PROP_NAME_TYPE,
									"alarm type",
									"type of the alarm",
									ALARM_TYPE_CLOCK,
									ALARM_TYPE_TIMER,
									ALARM_DEFAULT_TYPE,
									G_PARAM_READWRITE);
	
	
	time_param = g_param_spec_uint (PROP_NAME_TIME,
									 "alarm time",
									 "time when the alarm should trigger",
									 0,					/* min */
									 UINT_MAX,			/* max */
									 0,					/* default */
									 G_PARAM_READWRITE);

	message_param = g_param_spec_string (PROP_NAME_MESSAGE,
										 "alarm message",
										 "message which describes this alarm",
										 ALARM_DEFAULT_MESSAGE,
										 G_PARAM_READWRITE);
	
	notify_type_param = g_param_spec_uint (PROP_NAME_NOTIFY_TYPE,
										"notification type",
										"what kind of notification should be used",
										ALARM_NOTIFY_SOUND,
										ALARM_NOTIFY_COMMAND,
										ALARM_DEFAULT_NOTIFY_TYPE,
										G_PARAM_READWRITE);
	
	sound_file_param = g_param_spec_string (PROP_NAME_SOUND_FILE,
										 "sound file",
										 "sound file to play",
										 ALARM_DEFAULT_SOUND_FILE,
										 G_PARAM_READWRITE);
	
	sound_loop_param = g_param_spec_boolean (PROP_NAME_SOUND_LOOP,
											 "sound loop",
											 "whether the sound should be looped",
											 ALARM_DEFAULT_SOUND_LOOP,
											 G_PARAM_READWRITE);
	
	command_param = g_param_spec_string (PROP_NAME_COMMAND,
										 "command",
										 "command to run",
										 ALARM_DEFAULT_COMMAND,
										 G_PARAM_READWRITE);
	
	
	/* override base object methods */
	g_object_class->set_property = alarm_set_property;
	g_object_class->get_property = alarm_get_property;

	/* install properties */
	g_object_class_install_property (g_object_class, PROP_DIR, dir_param);
	g_object_class_install_property (g_object_class, PROP_ID, id_param);
	g_object_class_install_property (g_object_class, PROP_TYPE, type_param);
	g_object_class_install_property (g_object_class, PROP_TIME, time_param);
	g_object_class_install_property (g_object_class, PROP_MESSAGE, message_param);
	g_object_class_install_property (g_object_class, PROP_NOTIFY_TYPE, notify_type_param);
	g_object_class_install_property (g_object_class, PROP_SOUND_FILE, sound_file_param);
	g_object_class_install_property (g_object_class, PROP_SOUND_LOOP, sound_loop_param);
	g_object_class_install_property (g_object_class, PROP_COMMAND, command_param);
	
	/* set signal handlers */
	/*class->unpacked = media_unpacked;
	class->throw_out = media_throw_out;*/

	/* install signals and default handlers */
}

/* set an Alarm property */
static void 
alarm_set_property (GObject *object, 
								guint prop_id,
								const GValue *value,
								GParamSpec *pspec) 
{
	Alarm *alarm;
	
	GConfClient *client;
	GError 		*err = NULL;
	
	const gchar	*str;
	guint		 d;
	AlarmType 	 type, new_type;
	gboolean	 bool;
	
	gchar *key, *tmp;

	alarm = ALARM (object);
	client = gconf_client_get_default ();

	switch (prop_id) {
	case PROP_DIR:
		str = g_value_get_string (value);
		
		/* Validate */
		if (!str) {
			g_critical ("Invalid gconf-dir value: %s", str);
			return;
		}
		
		if (!gconf_valid_key(str, &tmp)) {
			g_critical ("Invalid gconf-dir value: \"%s\": %s", str, tmp);
			g_free (tmp);
			return;
		}
		
		/* TODO: Associate schemas if changed. */
		
		if (alarm->gconf_dir != NULL)
			g_free (alarm->gconf_dir);
		
		alarm->gconf_dir = g_strdup (str);
		break;
	case PROP_ID:
		alarm->id = g_value_get_uint (value);
		
		/* TODO: Associate schemas if id changed. */
		break;
	case PROP_TYPE:
		d = g_value_get_uint (value);
		
		key = alarm_gconf_get_full_key (alarm, PROP_NAME_TYPE);
		
		if (!gconf_client_set_string (client, key, 
									  alarm_type_to_string (d), 
									  &err)) {
			
			g_critical ("Could not set %s (gconf): %s", 
						key, err->message);
			
			g_error_free (err);
		}
		
		g_free (key);
		break;
	case PROP_TIME:
		d = g_value_get_uint (value);
		
		key = alarm_gconf_get_full_key (alarm, PROP_NAME_TIME);
		
		if (!gconf_client_set_int (client, key, d, &err)) {
			
			g_critical ("Could not set %s (gconf): %s", 
						key, err->message);
			
			g_error_free (err);
		}
		
		g_free (key);
		break;
	case PROP_MESSAGE:
		str = g_value_get_string (value);
		
		key = alarm_gconf_get_full_key (alarm, PROP_NAME_MESSAGE);
		
		if (!gconf_client_set_string (client, key, 
									  str, 
									  &err)) {
			
			g_critical ("Could not set %s (gconf): %s", 
						key, err->message);
			
			g_error_free (err);
		}
		
		g_free (key);
		
		break;
	case PROP_NOTIFY_TYPE:
		d = g_value_get_uint (value);
		
		key = alarm_gconf_get_full_key (alarm, PROP_NAME_NOTIFY_TYPE);
		
		if (!gconf_client_set_string (client, key, 
									  alarm_notify_type_to_string (d), 
									  &err)) {
			
			g_critical ("Could not set %s (gconf): %s", 
						key, err->message);
			
			g_error_free (err);
		}
		
		g_free (key);
		
		break;
	case PROP_SOUND_FILE:
		str = g_value_get_string (value);
		
		key = alarm_gconf_get_full_key (alarm, PROP_NAME_SOUND_FILE);
		
		if (!gconf_client_set_string (client, key, 
									  str, 
									  &err)) {
			
			g_critical ("Could not set %s (gconf): %s", 
						key, err->message);
			
			g_error_free (err);
		}
		
		g_free (key);
		
		break;
	case PROP_SOUND_LOOP:
		bool = g_value_get_boolean (value);
		
		key = alarm_gconf_get_full_key (alarm, PROP_NAME_SOUND_LOOP);
		
		if (!gconf_client_set_bool (client, key, bool, &err)) {
			
			g_critical ("Could not set %s (gconf): %s", 
						key, err->message);
			
			g_error_free (err);
		}
		
		g_free (key);
		break;
	case PROP_COMMAND:
		str = g_value_get_string (value);
		
		key = alarm_gconf_get_full_key (alarm, PROP_NAME_COMMAND);
		
		if (!gconf_client_set_string (client, key, 
									  str, 
									  &err)) {
			
			g_critical ("Could not set %s (gconf): %s", 
						key, err->message);
			
			g_error_free (err);
		}
		
		g_free (key);
		
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/* retrive an Alarm property */
static void 
alarm_get_property (GObject *object, 
					guint prop_id, 
					GValue *value,
					GParamSpec *pspec) 
{
	Alarm *alarm;
	
	GConfClient *client;
	GConfValue	*val;
	GError 		*err = NULL;
	
	gchar *key, *tmp;
	guint d;

	alarm = ALARM (object);
	client = gconf_client_get_default ();

	switch (prop_id) {
	case PROP_DIR:
		g_value_set_string (value, alarm->gconf_dir);
		
		break;
	case PROP_ID:
		g_value_set_uint (value, alarm->id);
		
		break;
	case PROP_TYPE:
		key = alarm_gconf_get_full_key (alarm, PROP_NAME_TYPE);
		
		tmp = gconf_client_get_string (client, key, NULL);
		g_free (key);
		
		if (!tmp) {
			// Not found in GConf, fall back to defaults
			g_value_set_uint (value, ALARM_DEFAULT_TYPE);
			g_object_set (alarm, PROP_NAME_TYPE, ALARM_DEFAULT_TYPE, NULL);
			return;
		}
		
		g_value_set_uint (value, alarm_type_from_string (tmp));
		g_free (tmp);
		
		break;
	case PROP_TIME:
		key = alarm_gconf_get_full_key (alarm, PROP_NAME_TIME);
		
		val = gconf_client_get (client, key, NULL);
		g_free (key);
		
		if (!val) {
			// Not found in GConf, fall back to defaults
			g_value_set_uint (value, ALARM_DEFAULT_TIME);
			g_object_set (alarm, PROP_NAME_TIME, ALARM_DEFAULT_TIME, NULL);
			return;
		}
		
		g_value_set_uint (value, gconf_value_get_int (val));
		gconf_value_free (val);
		
		break;
	case PROP_MESSAGE:
		key = alarm_gconf_get_full_key (alarm, PROP_NAME_MESSAGE);
		
		tmp = gconf_client_get_string (client, key, NULL);
		g_free (key);
		
		if (!tmp) {
			// Not found in GConf, fall back to defaults
			g_value_set_string (value, ALARM_DEFAULT_MESSAGE);
			g_object_set (alarm, PROP_NAME_MESSAGE, ALARM_DEFAULT_MESSAGE, NULL);
			return;
		}
		
		g_value_set_string (value, tmp);
		g_free (tmp);
		
		break;
	case PROP_NOTIFY_TYPE:
		key = alarm_gconf_get_full_key (alarm, PROP_NAME_NOTIFY_TYPE);
		
		tmp = gconf_client_get_string (client, key, NULL);
		g_free (key);
		
		if (!tmp) {
			// Not found in GConf, fall back to defaults
			g_value_set_uint (value, ALARM_DEFAULT_NOTIFY_TYPE);
			g_object_set (alarm, PROP_NAME_NOTIFY_TYPE, ALARM_DEFAULT_NOTIFY_TYPE, NULL);
			return;
		}
		
		g_value_set_uint (value, alarm_notify_type_from_string (tmp));
		g_free (tmp);
		
		break;
	case PROP_SOUND_FILE:
		key = alarm_gconf_get_full_key (alarm, PROP_NAME_SOUND_FILE);
		
		tmp = gconf_client_get_string (client, key, NULL);
		g_free (key);
		
		if (!tmp) {
			// Not found in GConf, fall back to defaults
			g_value_set_string (value, ALARM_DEFAULT_SOUND_FILE);
			g_object_set (alarm, PROP_NAME_SOUND_FILE, ALARM_DEFAULT_SOUND_FILE, NULL);
			return;
		}
		
		g_value_set_string (value, tmp);
		g_free (tmp);
		
		break;
	case PROP_SOUND_LOOP:
		key = alarm_gconf_get_full_key (alarm, PROP_NAME_SOUND_LOOP);
		
		val = gconf_client_get (client, key, NULL);
		g_free (key);
		
		if (!val) {
			// Not found in GConf, fall back to defaults
			g_value_set_boolean (value, ALARM_DEFAULT_SOUND_LOOP);
			g_object_set (alarm, PROP_NAME_SOUND_LOOP, ALARM_DEFAULT_SOUND_LOOP, NULL);
			return;
		}
		
		g_value_set_boolean (value, gconf_value_get_bool (val));
		gconf_value_free (val);
		
		break;
	case PROP_COMMAND:
		key = alarm_gconf_get_full_key (alarm, PROP_NAME_COMMAND);
		
		tmp = gconf_client_get_string (client, key, NULL);
		g_free (key);
		
		if (!tmp) {
			// Not found in GConf, fall back to defaults
			g_value_set_string (value, ALARM_DEFAULT_COMMAND);
			g_object_set (alarm, PROP_NAME_COMMAND, ALARM_DEFAULT_COMMAND, NULL);
			return;
		}
		
		g_value_set_string (value, tmp);
		g_free (tmp);
		
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

const gchar *alarm_type_to_string (AlarmType type)
{
	return gconf_enum_to_string (alarm_type_enum_map, type);
}

AlarmType alarm_type_from_string (const gchar *type)
{
	AlarmType ret = ALARM_TYPE_INVALID;
	
	gconf_string_to_enum (alarm_type_enum_map, type, (gint *)&ret);
	
	return ret;
}

const gchar *alarm_notify_type_to_string (AlarmNotifyType type)
{
	return gconf_enum_to_string (alarm_notify_type_enum_map, type);
}

AlarmNotifyType alarm_notify_type_from_string (const gchar *type)
{
	AlarmNotifyType ret = ALARM_NOTIFY_INVALID;
	
	gconf_string_to_enum (alarm_notify_type_enum_map, type, (gint *)&ret);
	
	return ret;
}

gchar *alarm_gconf_get_full_key (Alarm *alarm, const gchar *key)
{
	gchar *full_key;

	g_return_val_if_fail (IS_ALARM (alarm), NULL);

	if (!key)
		return NULL;
	
	full_key = g_strdup_printf ("%s/alarm%d/%s", alarm->gconf_dir, alarm->id, key);

	return full_key;
}
