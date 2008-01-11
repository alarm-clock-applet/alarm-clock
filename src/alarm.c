/*
 * Copyright/Licensing information.
 */

#include <stdio.h>

#include "alarm.h"

G_DEFINE_TYPE (Alarm, alarm, G_TYPE_OBJECT);

/* Prototypes and constants for property manipulation */
static void alarm_set_property(GObject *object,
                               guint prop_id,
                               const GValue *value,
                               GParamSpec *pspec);

static void alarm_get_property(GObject *object,
                               guint prop_id,
                               GValue *value,
                               GParamSpec *pspec);

static void alarm_constructed (GObject *object);

static void alarm_dispose (GObject *object);

static void alarm_gconf_connect (Alarm *alarm);

static void alarm_gconf_disconnect (Alarm *alarm);

static void alarm_gconf_dir_changed (GConfClient *client,
									 guint cnxn_id,
									 GConfEntry *entry,
									 gpointer data);

#define ALARM_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ALARM, AlarmPrivate))

typedef struct _AlarmPrivate AlarmPrivate;

struct _AlarmPrivate
{
	GConfClient *gconf_client;
	guint gconf_listener;
};


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

/* Property indexes */
static enum {
	PROP_ALARM_0,
	PROP_DIR,
	PROP_ID,
	PROP_TYPE,
	PROP_TIME,
	PROP_ACTIVE,
	PROP_MESSAGE,
	PROP_NOTIFY_TYPE,
	PROP_SOUND_FILE,
	PROP_SOUND_LOOP,
	PROP_COMMAND
} AlarmProp;

#define PROP_NAME_DIR			"gconf-dir"
#define PROP_NAME_ID			"id"
#define PROP_NAME_TYPE			"type"
#define PROP_NAME_TIME			"time"
#define PROP_NAME_ACTIVE		"active"
#define PROP_NAME_MESSAGE		"message"
#define PROP_NAME_NOTIFY_TYPE	"notify_type"
#define PROP_NAME_SOUND_FILE	"sound_file"
#define PROP_NAME_SOUND_LOOP	"sound_repeat"
#define PROP_NAME_COMMAND		"command"

/* Signal indexes */
static enum
{
  SIGNAL_ALARM,
  LAST_SIGNAL
} AlarmSignal;

/* Signal identifier map */
static guint alarm_signal[LAST_SIGNAL] = {0};

/* Prototypes for signal handlers */
static void alarm_alarm (Alarm *alarm);


/* Initialize the Alarm class */
static void 
alarm_class_init (AlarmClass *class)
{
	GParamSpec *dir_param;
	GParamSpec *id_param;
	GParamSpec *type_param;
	GParamSpec *time_param;
	GParamSpec *active_param;
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
	
	active_param = g_param_spec_boolean (PROP_NAME_ACTIVE, 
										 "active alarm",
										 "whether the alarm is on or not",
										 ALARM_DEFAULT_ACTIVE,
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
	g_object_class->constructed	 = alarm_constructed;
	g_object_class->dispose		 = alarm_dispose;

	/* install properties */
	g_object_class_install_property (g_object_class, PROP_DIR, dir_param);
	g_object_class_install_property (g_object_class, PROP_ID, id_param);
	g_object_class_install_property (g_object_class, PROP_TYPE, type_param);
	g_object_class_install_property (g_object_class, PROP_TIME, time_param);
	g_object_class_install_property (g_object_class, PROP_ACTIVE, active_param);
	g_object_class_install_property (g_object_class, PROP_MESSAGE, message_param);
	g_object_class_install_property (g_object_class, PROP_NOTIFY_TYPE, notify_type_param);
	g_object_class_install_property (g_object_class, PROP_SOUND_FILE, sound_file_param);
	g_object_class_install_property (g_object_class, PROP_SOUND_LOOP, sound_loop_param);
	g_object_class_install_property (g_object_class, PROP_COMMAND, command_param);
	
	g_type_class_add_private (class, sizeof (AlarmPrivate));
	
	/* set signal handlers */
	class->alarm = alarm_alarm;

	/* install signals and default handlers */
	alarm_signal[SIGNAL_ALARM] = g_signal_new ("alarm",		/* name */
											   TYPE_ALARM,	/* class type identifier */
											   G_SIGNAL_RUN_LAST, /* options */
											   G_STRUCT_OFFSET (AlarmClass, alarm), /* handler offset */
											   NULL, /* accumulator function */
											   NULL, /* accumulator data */
											   g_cclosure_marshal_VOID__VOID, /* marshaller */
											   G_TYPE_NONE, /* type of return value */
											   0);
}

static void
alarm_gconf_load (Alarm *alarm)
{
	AlarmPrivate *priv	= ALARM_PRIVATE (alarm);
	GConfClient *client = priv->gconf_client;
	GConfValue *val;
	gchar *key, *tmp;
	guint i;
	
	/*
	 * TYPE
	 */
	key = alarm_gconf_get_full_key (alarm, PROP_NAME_TYPE);
	tmp = gconf_client_get_string (client, key, NULL);
	g_free (key);
	
	i = alarm_type_from_string (tmp);
	
	if (i > 0) {
		alarm->type = i;
	} else {
		// Not found in GConf, fall back to defaults
		alarm->type = ALARM_DEFAULT_TYPE;
		g_object_set (alarm, PROP_NAME_TYPE, ALARM_DEFAULT_TYPE, NULL);
	}
	
	if (tmp)
		g_free (tmp);
	
	/*
	 * TIME
	 */
	key = alarm_gconf_get_full_key (alarm, PROP_NAME_TIME);
	val = gconf_client_get (client, key, NULL);
	g_free (key);
	
	if (val) {
		alarm->time = (time_t)gconf_value_get_int (val);
		gconf_value_free (val);
	} else {
		// Not found in GConf, fall back to defaults
		g_object_set (alarm, PROP_NAME_TIME, ALARM_DEFAULT_TIME, NULL);
	}
	
	/*
	 * ACTIVE
	 */
	key = alarm_gconf_get_full_key (alarm, PROP_NAME_ACTIVE);
	val = gconf_client_get (client, key, NULL);
	g_free (key);
	
	if (val) {
		alarm->active = gconf_value_get_bool (val);
		gconf_value_free (val);
	} else {
		// Not found in GConf, fall back to defaults
		g_object_set (alarm, PROP_NAME_ACTIVE, ALARM_DEFAULT_ACTIVE, NULL);
	}
	
	/*
	 * MESSAGE
	 */
	key = alarm_gconf_get_full_key (alarm, PROP_NAME_MESSAGE);
	tmp = gconf_client_get_string (client, key, NULL);
	g_free (key);
	
	if (tmp) {
		alarm->message = tmp;
	} else {
		// Not found in GConf, fall back to defaults
		g_object_set (alarm, PROP_NAME_MESSAGE, ALARM_DEFAULT_MESSAGE, NULL);
	}
	
	/*
	 * NOTIFY TYPE
	 */
	key = alarm_gconf_get_full_key (alarm, PROP_NAME_NOTIFY_TYPE);
	tmp = gconf_client_get_string (client, key, NULL);
	g_free (key);
	
	i = alarm_notify_type_from_string (tmp);
	
	if (i > 0) {
		alarm->notify_type = i;
	} else {
		// Not found in GConf, fall back to defaults
		g_object_set (alarm, PROP_NAME_NOTIFY_TYPE, ALARM_DEFAULT_NOTIFY_TYPE, NULL);
	}
	
	if (tmp)
		g_free (tmp);
	
	/*
	 * SOUND FILE
	 */
	key = alarm_gconf_get_full_key (alarm, PROP_NAME_SOUND_FILE);
	tmp = gconf_client_get_string (client, key, NULL);
	g_free (key);
	
	if (tmp) {
		alarm->sound_file = tmp;
	} else {
		// Not found in GConf, fall back to defaults
		g_object_set (alarm, PROP_NAME_SOUND_FILE, ALARM_DEFAULT_SOUND_FILE, NULL);
	}
	
	/*
	 * SOUND LOOP
	 */
	key = alarm_gconf_get_full_key (alarm, PROP_NAME_SOUND_LOOP);
	val = gconf_client_get (client, key, NULL);
	g_free (key);
	
	if (val) {
		alarm->sound_loop = gconf_value_get_bool (val);
		gconf_value_free (val);
	} else {
		// Not found in GConf, fall back to defaults
		g_object_set (alarm, PROP_NAME_SOUND_LOOP, ALARM_DEFAULT_SOUND_LOOP, NULL);
	}


	/*
	 * COMMAND
	 */
	key = alarm_gconf_get_full_key (alarm, PROP_NAME_COMMAND);
	tmp = gconf_client_get_string (client, key, NULL);
	g_free (key);
	
	if (tmp) {
		alarm->command = tmp;
	} else {
		// Not found in GConf, fall back to defaults
		g_object_set (alarm, PROP_NAME_COMMAND, ALARM_DEFAULT_COMMAND, NULL);
	}
}

static void
alarm_init (Alarm *self)
{
	AlarmPrivate *priv = ALARM_PRIVATE (self);
	
	self->gconf_dir = NULL;
	self->id = -1;
	
	priv->gconf_listener = 0;
	priv->gconf_client = gconf_client_get_default ();
}

static void
alarm_constructed (GObject *object)
{
	Alarm *alarm = ALARM (object);
	
	// Load gconf settings
	alarm_gconf_load (alarm);
	
	// Connect gconf listener
	//alarm_gconf_connect (alarm);
}

/* set an Alarm property */
static void 
alarm_set_property (GObject *object, 
					guint prop_id,
					const GValue *value,
					GParamSpec *pspec) 
{
	Alarm *alarm;
	AlarmPrivate *priv = ALARM_PRIVATE (object);
	
	GConfClient *client;
	GError 		*err = NULL;
	
	const gchar	*str;
	guint		 d, old;
	AlarmType 	 type, new_type;
	gboolean	 bool;
	
	gchar *key, *tmp;

	alarm = ALARM (object);
	client = priv->gconf_client;
	
	switch (prop_id) {
	case PROP_DIR:
		str = g_value_get_string (value);
		
		/* Validate */
		if (!str) {
			g_critical ("Invalid gconf-dir value: \"%s\": NULL", str);
			return;
		}
		
		if (!gconf_valid_key (str, &tmp)) {
			g_critical ("Invalid gconf-dir value: \"%s\": %s", str, tmp);
			g_free (tmp);
			return;
		}
		
		/* TODO: Associate schemas if changed. */
		if (!alarm->gconf_dir || strcmp (str, alarm->gconf_dir) != 0) {
			// Changed, remove old gconf listeners
			alarm_gconf_disconnect (alarm);
			
			if (alarm->gconf_dir != NULL)
				g_free (alarm->gconf_dir);
			alarm->gconf_dir = g_strdup (str);
			
			alarm_gconf_connect (alarm);
		}
		break;
	case PROP_ID:
		d = g_value_get_uint (value);
		
		/* TODO: Associate schemas if id changed. */
		if (d != alarm->id) {
			alarm_gconf_disconnect (alarm);
			alarm->id = d;
			alarm_gconf_connect (alarm);
		}
		break;
	case PROP_TYPE:
		alarm->type = g_value_get_uint (value);
		
		key = alarm_gconf_get_full_key (alarm, PROP_NAME_TYPE);
		
		if (!gconf_client_set_string (client, key, 
									  alarm_type_to_string (alarm->type), 
									  &err)) {
			
			g_critical ("Could not set %s (gconf): %s", 
						key, err->message);
			
			g_error_free (err);
		}
		
		g_free (key);
		break;
	case PROP_TIME:
		alarm->time = g_value_get_uint (value);
		
		key = alarm_gconf_get_full_key (alarm, PROP_NAME_TIME);
		
		if (!gconf_client_set_int (client, key, alarm->time, &err)) {
			
			g_critical ("Could not set %s (gconf): %s", 
						key, err->message);
			
			g_error_free (err);
		}
		
		g_free (key);
		break;
	case PROP_ACTIVE:
		alarm->active = g_value_get_boolean (value);
		
		key = alarm_gconf_get_full_key (alarm, PROP_NAME_ACTIVE);
		
		if (!gconf_client_set_bool (client, key, alarm->active, &err)) {
			
			g_critical ("Could not set %s (gconf): %s", 
						key, err->message);
			
			g_error_free (err);
		}
		
		g_free (key);
		break;
	case PROP_MESSAGE:
		if (alarm->message)
			g_free (alarm->message);
		
		alarm->message = g_strdup (g_value_get_string (value));
		
		key = alarm_gconf_get_full_key (alarm, PROP_NAME_MESSAGE);
		
		if (!gconf_client_set_string (client, key, 
									  alarm->message, 
									  &err)) {
			
			g_critical ("Could not set %s (gconf): %s", 
						key, err->message);
			
			g_error_free (err);
		}
		
		g_free (key);
		
		break;
	case PROP_NOTIFY_TYPE:
		alarm->notify_type = g_value_get_uint (value);
		
		key = alarm_gconf_get_full_key (alarm, PROP_NAME_NOTIFY_TYPE);
		
		if (!gconf_client_set_string (client, key, 
									  alarm_notify_type_to_string (alarm->notify_type), 
									  &err)) {
			
			g_critical ("Could not set %s (gconf): %s", 
						key, err->message);
			
			g_error_free (err);
		}
		
		g_free (key);
		
		break;
	case PROP_SOUND_FILE:
		alarm->sound_file = g_strdup (g_value_get_string (value));
		
		key = alarm_gconf_get_full_key (alarm, PROP_NAME_SOUND_FILE);
		
		if (!gconf_client_set_string (client, key, 
									  alarm->sound_file, 
									  &err)) {
			
			g_critical ("Could not set %s (gconf): %s", 
						key, err->message);
			
			g_error_free (err);
		}
		
		g_free (key);
		
		break;
	case PROP_SOUND_LOOP:
		alarm->sound_loop = g_value_get_boolean (value);
		
		key = alarm_gconf_get_full_key (alarm, PROP_NAME_SOUND_LOOP);
		
		if (!gconf_client_set_bool (client, key, alarm->sound_loop, &err)) {
			
			g_critical ("Could not set %s (gconf): %s", 
						key, err->message);
			
			g_error_free (err);
		}
		
		g_free (key);
		break;
	case PROP_COMMAND:
		alarm->command = g_strdup (g_value_get_string (value));
		
		key = alarm_gconf_get_full_key (alarm, PROP_NAME_COMMAND);
		
		if (!gconf_client_set_string (client, key, 
									  alarm->command, 
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
	Alarm *alarm = ALARM (object);
	
	switch (prop_id) {
	case PROP_DIR:
		g_value_set_string (value, alarm->gconf_dir);
		break;
	case PROP_ID:
		g_value_set_uint (value, alarm->id);
		break;
	case PROP_TYPE:
		g_value_set_uint (value, alarm->type);
		break;
	case PROP_TIME:
		g_value_set_uint (value, alarm->time);
		break;
	case PROP_ACTIVE:
		g_value_set_boolean (value, alarm->active);
		break;
	case PROP_MESSAGE:
		g_value_set_string (value, alarm->message);
		break;
	case PROP_NOTIFY_TYPE:
		g_value_set_uint (value, alarm->notify_type);
		break;
	case PROP_SOUND_FILE:
		g_value_set_string (value, alarm->sound_file);
		break;
	case PROP_SOUND_LOOP:
		g_value_set_boolean (value, alarm->sound_loop);
		break;
	case PROP_COMMAND:
		g_value_set_string (value, alarm->command);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void 
alarm_alarm (Alarm *alarm)
{
	g_debug ("alarm_alarm (%p)", alarm);
}

void
alarm_trigger (Alarm *alarm)
{
	g_signal_emit (alarm, alarm_signal[SIGNAL_ALARM], 0, NULL);
}

static void
alarm_gconf_connect (Alarm *alarm)
{
	AlarmPrivate *priv = ALARM_PRIVATE (alarm);
	gchar *dir;
	
//	g_debug ("gconf_connect (%p) ? %d", alarm, IS_ALARM ((gpointer)alarm));
	
	dir = alarm_gconf_get_dir (alarm);
	
	gconf_client_add_dir (priv->gconf_client, dir, 
						  GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
	
	priv->gconf_listener = 
		gconf_client_notify_add (
			priv->gconf_client, dir,
			(GConfClientNotifyFunc) alarm_gconf_dir_changed,
			alarm, NULL, NULL);
	
	g_free (dir);
}

static void
alarm_gconf_disconnect (Alarm *alarm)
{
	AlarmPrivate *priv = ALARM_PRIVATE (alarm);
	gchar *dir;
	
	if (priv->gconf_listener) {
		gconf_client_notify_remove (priv->gconf_client, priv->gconf_listener);
		
		dir = alarm_gconf_get_dir (alarm);
		gconf_client_remove_dir (priv->gconf_client, dir, NULL);
		g_free (dir);
	}
}

/*
 * Updates the local copy with the new value if it has changed.
 */
static void 
alarm_gconf_dir_changed (GConfClient *client,
						 guint cnxn_id,
						 GConfEntry *entry,
						 gpointer data)
{
	Alarm *alarm = ALARM (data);
	GParamSpec *param;
	gchar *name;
	
	guint i;
	gboolean b;
	const gchar *str;
	
	name = g_path_get_basename (entry->key);
	param = g_object_class_find_property (G_OBJECT_GET_CLASS (alarm), name);
	
	if (!param) {
		g_free (name);
		return;
	}
	
//	g_debug ("alarm_gconf changed: %s", name);
	
	switch (param->param_id) {
	case PROP_TYPE:
		str = gconf_value_get_string (entry->value);
		i = alarm_type_from_string (str);
		if (i > 0 && i != alarm->type)
			g_object_set (alarm, name, i, NULL);
		break;
	case PROP_TIME:
		i = gconf_value_get_int (entry->value);
		if (i != alarm->time)
			g_object_set (alarm, name, i, NULL);
		break;
	case PROP_ACTIVE:
		b = gconf_value_get_bool (entry->value);
		if (b != alarm->active)
			g_object_set (alarm, name, b, NULL);
		break;
	case PROP_MESSAGE:
		str = gconf_value_get_string (entry->value);
		if (strcmp (str, alarm->message) != 0)
			g_object_set (alarm, name, str, NULL);
		break;
	case PROP_NOTIFY_TYPE:
		str = gconf_value_get_string (entry->value);
		i = alarm_notify_type_from_string (str);
		if (i > 0 && i != alarm->notify_type)
			g_object_set (alarm, name, i, NULL);
		break;
	case PROP_SOUND_FILE:
		str = gconf_value_get_string (entry->value);
		if (strcmp (str, alarm->sound_file) != 0)
			g_object_set (alarm, name, str, NULL);
		break;
	case PROP_SOUND_LOOP:
		b = gconf_value_get_bool (entry->value);
		if (b != alarm->sound_loop)
			g_object_set (alarm, name, b, NULL);
		break;
	case PROP_COMMAND:
		str = gconf_value_get_string (entry->value);
		if (strcmp (str, alarm->command) != 0)
			g_object_set (alarm, name, str, NULL);
		break;
	default:
		g_warning ("Valid property ID %d not handled!", param->param_id);
		break;
	}
	
	g_free (name);
}

static void
alarm_dispose (GObject *object)
{
	Alarm *alarm = ALARM (object);
	AlarmPrivate *priv = ALARM_PRIVATE (object);
	GObjectClass *parent = (GObjectClass *)alarm_parent_class;
	
	if (parent->dispose)
		parent->dispose (object);
	
	alarm_gconf_disconnect (alarm);
}

/*
 * Convenience function for creating a new alarm instance.
 * Passing -1 as the id will generate a new ID with alarm_gen_id
 */
Alarm *
alarm_new (const gchar *gconf_dir, gint id)
{
	Alarm *alarm;
	
	if (id < 0) {
		id = alarm_gen_id_dir (gconf_dir);
	}
	
	alarm = g_object_new (TYPE_ALARM,
						  "gconf-dir", gconf_dir,
						  "id", id,
						  NULL);
	
	return alarm;
}

guint
alarm_gen_id_dir (const gchar *gconf_dir)
{
	GConfClient *client;
	gchar *key = NULL;
	gint id;
	
	client = gconf_client_get_default ();
	
	id = 0;
	do {
		if (key)
			g_free (key);
		
		key = g_strdup_printf("%s/" ALARM_GCONF_DIR_PREFIX "%d", gconf_dir, id);
		id++;
		
	} while (gconf_client_dir_exists (client, key, NULL));
	
	g_free (key);
	
	return (guint)(id-1);
}

/*
 * Will try to find the first available ID in gconf_dir
 */
guint
alarm_gen_id (Alarm *alarm)
{
	return alarm_gen_id_dir (alarm->gconf_dir);
}

const gchar *alarm_type_to_string (AlarmType type)
{
	return gconf_enum_to_string (alarm_type_enum_map, type);
}

AlarmType alarm_type_from_string (const gchar *type)
{
	AlarmType ret = ALARM_TYPE_INVALID;
	
	g_return_val_if_fail(type, ret);
	
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
	
	g_return_val_if_fail (type, ret);
	
	gconf_string_to_enum (alarm_notify_type_enum_map, type, (gint *)&ret);
	
	return ret;
}

gchar *
alarm_gconf_get_dir (Alarm *alarm)
{
	gchar *key;
	
	g_return_val_if_fail (IS_ALARM (alarm), NULL);
	
	key = g_strdup_printf ("%s/" ALARM_GCONF_DIR_PREFIX "%d", alarm->gconf_dir, alarm->id);
	
	return key;
}

gchar *
alarm_gconf_get_full_key (Alarm *alarm, const gchar *key)
{
	gchar *full_key;
	
	g_return_val_if_fail (IS_ALARM (alarm), NULL);

	if (!key)
		return NULL;
	
	full_key = g_strdup_printf ("%s/" ALARM_GCONF_DIR_PREFIX "%d/%s", alarm->gconf_dir, alarm->id, key);

	return full_key;
}

typedef struct {
	GObject *object;
	const gchar *name;
} AlarmBindArg;

static AlarmBindArg *
alarm_bind_arg_new (GObject *object, const gchar *name)
{
	AlarmBindArg *arg;
	
	arg = g_new (AlarmBindArg, 1);
	arg->object = object;
	arg->name = name;
	
	return arg;
}

static void
alarm_bind_arg_free (AlarmBindArg *arg)
{
	if (!arg)
		return;
	
	g_free ((gpointer)arg);
}

static void
alarm_bind_update (GObject *object,
				   GParamSpec *pspec,
				   gpointer data)
{
	Alarm *alarm	  = ALARM (object);
	AlarmBindArg *arg = (AlarmBindArg *)data;
	gpointer d;
	
	g_debug ("alarm_bind_update %p [%s] -> %p [%s]", object, pspec->name, arg->object, arg->name);
	
	g_object_get (object, pspec->name, &d, NULL);
	
	// Block other signal handler
	g_signal_handlers_block_matched(arg->object, 
									G_SIGNAL_MATCH_FUNC,
									0, 0, NULL, alarm_bind_update, NULL);
	
	g_object_set (arg->object, arg->name, d, NULL);
	
	g_signal_handlers_unblock_matched(arg->object, 
									  G_SIGNAL_MATCH_FUNC,
									  0, 0, NULL, alarm_bind_update, NULL);
}

/*
 * Binds both ways.
 */
void
alarm_bind (Alarm *alarm, 
			const gchar *prop, 
			GObject *dest, 
			const gchar *dest_prop)
{
	AlarmBindArg *arg;
	GParamSpec *param;
	gchar *tmp;
	
	param = g_object_class_find_property (G_OBJECT_GET_CLASS(alarm), prop);
	g_assert (param);
	param = g_object_class_find_property (G_OBJECT_GET_CLASS(dest), dest_prop);
	g_assert (param);
	
//	g_debug ("Bind from %p [%s] -> %p [%s]", alarm, prop, dest, dest_prop);
	
	// Alarm -> Object
	tmp = g_strdup_printf("notify::%s", prop);
	arg = alarm_bind_arg_new (dest, dest_prop);
	g_signal_connect (alarm, tmp, G_CALLBACK (alarm_bind_update), arg);
	
	// Object -> Alarm
	tmp = g_strdup_printf ("notify::%s", dest_prop);
	arg = alarm_bind_arg_new (G_OBJECT (alarm), prop);
	g_signal_connect (dest, tmp, G_CALLBACK (alarm_bind_update), arg);
}

static gint
alarm_list_item_compare (gconstpointer a, gconstpointer b)
{
	return strcmp ((const gchar *)a, (const gchar *)b);
}

/*
 * Get list of alarms in gconf_dir
 */
GList *
alarm_get_list (const gchar *gconf_dir)
{
	GConfClient *client;
	GSList *dirs = NULL;
	GSList *l = NULL;
	gchar *tmp, *dir;
	
	GList *ret = NULL;
	Alarm *alarm;
	gint id;
	
	client = gconf_client_get_default ();
	dirs = gconf_client_all_dirs (client, gconf_dir, NULL);
	
	g_return_val_if_fail (dirs, NULL);
	
	dirs = g_slist_sort (dirs, alarm_list_item_compare);
	
	for (l = dirs; l; l = l->next) {
		tmp = (gchar *)l->data;
		dir = g_path_get_basename (tmp);
		
		if (sscanf (dir, ALARM_GCONF_DIR_PREFIX "%d", &id) > 0 && id >= 0) {
			g_debug ("alarm_get_list: found VALID %s #%d", dir, id);
			
			alarm = alarm_new (gconf_dir, id);
			ret = g_list_append (ret, alarm);
		}
		
		g_free (dir);
		g_free (tmp);
	}
	
	g_slist_free (dirs);
	
	return ret;
}


