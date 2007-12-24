/*
 * 
 * ALARM APPLET Gconf callbacks and utilities.
 * 
 */

#include <time.h>
#include <panel-applet-gconf.h>

#include "alarm-applet.h"
#include "alarm-gconf.h"

GConfEnumStringPair label_type_enum_map [] = {
	{ LABEL_TYPE_ALARM,		"alarm-time"  },
	{ LABEL_TYPE_REMAIN,	"remaining-time"  },
	{ 0, NULL }
};

GConfEnumStringPair notify_type_enum_map [] = {
	{ NOTIFY_SOUND,			"sound"  },
	{ NOTIFY_COMMAND,		"command" },
	{ 0, NULL }
};

/*
 * SETTERS {{
 */

/**
 * Stores the alarm timestamp in gconf
 */
void
alarm_gconf_set_alarm (AlarmApplet *applet, guint hour, guint minute, guint second)
{
	// Get timestamp
	time_t timestamp = get_alarm_timestamp(hour, minute, second);
	
	// Store in GConf
	panel_applet_gconf_set_int (applet->parent, KEY_ALARMTIME, (gint)timestamp, NULL);	
}

/**
 * Stores the alarm message in gconf
 */
void
alarm_gconf_set_message (AlarmApplet *applet, const gchar *message)
{
	g_debug ("set_alarm_message %s", message);
	
	if (message == NULL)
		return;
	
	panel_applet_gconf_set_string (applet->parent, KEY_MESSAGE, message, NULL);
}

/**
 * Sets started status of alarm in gconf
 */
void
alarm_gconf_set_started (AlarmApplet *applet, gboolean started)
{
	// Store in GConf
	panel_applet_gconf_set_bool (applet->parent, KEY_STARTED, started, NULL);	
}

/*
 * }} SETTERS
 */

/*
 * GCONF CALLBACKS {{
 */

void
alarm_gconf_alarmtime_changed (GConfClient  *client,
						 guint         cnxn_id,
						 GConfEntry   *entry,
						 AlarmApplet  *applet)
{
	g_debug ("alarmtime_changed");
	
	time_t value;
	struct tm *tm;
	
	if (!entry->value || entry->value->type != GCONF_VALUE_INT)
		return;
	
	value = (time_t) gconf_value_get_int (entry->value);
	
	if (applet->started) {
		// We have already started, start timer
		
		applet->alarm_time = value;
		
		timer_start (applet);
	} else {
		// Fetch values
		// We're only interested in the hour, minute and second fields
		tm = localtime (&value);
		
		applet->alarm_time = get_alarm_timestamp (tm->tm_hour, tm->tm_min, tm->tm_sec);
		
		if (applet->set_alarm_dialog != NULL) {
			set_alarm_dialog_populate (applet);
			return;
		}
	}
	
	update_label (applet);
}

void
alarm_gconf_message_changed (GConfClient  *client,
					   guint         cnxn_id,
					   GConfEntry   *entry,
					   AlarmApplet  *applet)
{
	g_debug ("message_changed");
	
	const gchar *value;
	
	if (!entry->value || entry->value->type != GCONF_VALUE_STRING)
		return;

	value = gconf_value_get_string (entry->value);
	
	if (applet->alarm_message != NULL) {
		g_free (applet->alarm_message);
		applet->alarm_message = NULL;
	}
	
	applet->alarm_message = g_strdup (value);
	
	if (applet->set_alarm_dialog != NULL) {
		set_alarm_dialog_populate (applet);
	}
}


void
alarm_gconf_started_changed (GConfClient  *client,
					   guint         cnxn_id,
					   GConfEntry   *entry,
					   AlarmApplet  *applet)
{
	g_debug ("started_changed");
	
	gboolean value;
	time_t now;
	
	if (!entry->value || entry->value->type != GCONF_VALUE_BOOL)
		return;
	
	value = gconf_value_get_bool (entry->value);
	time (&now);
	
	if (value && applet->alarm_time >= now) {
		// Start timer
		timer_start (applet);
		applet->started = TRUE;
	} else {
		// Stop timer
		timer_remove (applet);
		applet->started = FALSE;
	}
	
	update_label (applet);
}

void
alarm_gconf_show_label_changed (GConfClient  *client,
						  guint         cnxn_id,
						  GConfEntry   *entry,
						  AlarmApplet  *applet)
{
	g_debug ("show_label_changed");
	
	if (!entry->value || entry->value->type != GCONF_VALUE_BOOL)
		return;
	
	applet->show_label = gconf_value_get_bool (entry->value);
	
	g_object_set (applet->label, "visible", applet->show_label, NULL);
	
	if (applet->preferences_dialog != NULL) {
		pref_update_label_show (applet);
	}
}

void
alarm_gconf_label_type_changed (GConfClient  *client,
						  guint         cnxn_id,
						  GConfEntry   *entry,
						  AlarmApplet  *applet)
{
	g_debug ("label_type_changed");
	
	const gchar *tmp;
	
	if (!entry->value || entry->value->type != GCONF_VALUE_STRING)
		return;
	
	tmp = gconf_value_get_string (entry->value);
	if (tmp) {
		if (!gconf_string_to_enum (label_type_enum_map, tmp, (gint *)&(applet->label_type))) {
			// No match, set to default
			applet->label_type = LABEL_TYPE_ALARM;
		}
		
		update_label (applet);
	}
	
	if (applet->preferences_dialog != NULL) {
		pref_update_label_type (applet);
	}
}

void
alarm_gconf_notify_type_changed (GConfClient  *client,
						   guint         cnxn_id,
						   GConfEntry   *entry,
						   AlarmApplet  *applet)
{
	g_debug ("notify_type_changed");
	
	const gchar *tmp;
	
	if (!entry->value || entry->value->type != GCONF_VALUE_STRING)
		return;
	
	tmp = gconf_value_get_string (entry->value);
	if (tmp) {
		if (!gconf_string_to_enum (notify_type_enum_map, tmp, (gint *)&(applet->notify_type))) {
			// No match, set to default
			applet->notify_type = NOTIFY_SOUND;
		}
	}
	
	if (applet->preferences_dialog != NULL) {
		pref_update_notify_type (applet);
	}
}


void
alarm_gconf_sound_file_changed (GConfClient  *client,
						  guint         cnxn_id,
						  GConfEntry   *entry,
						  AlarmApplet  *applet)
{
	if (!entry->value || entry->value->type != GCONF_VALUE_STRING)
		return;
	
	const gchar *value = gconf_value_get_string (entry->value);
	
	g_debug ("sound_file_changed to %s", value);
	
	if (set_sound_file (applet, value)) {
		g_debug ("VALID file!");
	}
	
	if (applet->preferences_dialog != NULL) {
		pref_update_sound_file (applet);
	}
}

void
alarm_gconf_sound_loop_changed (GConfClient  *client,
						  guint         cnxn_id,
						  GConfEntry   *entry,
						  AlarmApplet  *applet)
{
	g_debug ("sound_loop_changed");
	
	if (!entry->value || entry->value->type != GCONF_VALUE_BOOL)
		return;
	
	applet->notify_sound_loop = gconf_value_get_bool (entry->value);
	
	if (applet->player)
		applet->player->loop = applet->notify_sound_loop;
	
	if (applet->preferences_dialog != NULL) {
		pref_update_sound_loop (applet);
	}
}



void
alarm_gconf_command_changed (GConfClient  *client,
					   guint         cnxn_id,
					   GConfEntry   *entry,
					   AlarmApplet  *applet)
{
	g_debug ("[gconf] command_changed");
	
	if (!entry->value || entry->value->type != GCONF_VALUE_STRING)
		return;
	
	applet->notify_command = (gchar *)gconf_value_get_string (entry->value);
	
	if (applet->preferences_dialog != NULL) {
		g_debug ("gconf: CMD ENTRY HAS FOCUS? %d", GTK_WIDGET_HAS_FOCUS (applet->pref_notify_app_command_entry));
		pref_update_command (applet);
	}
}

void
alarm_gconf_notify_bubble_change (GConfClient  *client,
							 guint         cnxn_id,
							 GConfEntry   *entry,
							 AlarmApplet  *applet)
{
	g_debug ("notify_bubble_changed");
	
#ifdef HAVE_LIBNOTIFY
	if (!entry->value || entry->value->type != GCONF_VALUE_BOOL)
		return;
	
	applet->notify_bubble = gconf_value_get_bool (entry->value);
	
	if (applet->preferences_dialog != NULL) {
		pref_update_show_bubble (applet);
	}
#endif
}


/*
 * }} GCONF CALLBACKS
 */

/*
 * Init
 */
void
setup_gconf (AlarmApplet *applet)
{
	GConfClient *client;
	gchar       *key;

	client = gconf_client_get_default ();
	
	key = panel_applet_gconf_get_full_key (PANEL_APPLET (applet->parent), KEY_ALARMTIME);
	g_debug ("GCONFKEY: %s", key);
	applet->listeners [0] =
		gconf_client_notify_add (
				client, key,
				(GConfClientNotifyFunc) alarm_gconf_alarmtime_changed,
				applet, NULL, NULL);
	g_free (key);
	
	key = panel_applet_gconf_get_full_key (PANEL_APPLET (applet->parent), KEY_MESSAGE);
	applet->listeners [1] =
		gconf_client_notify_add (
				client, key,
				(GConfClientNotifyFunc) alarm_gconf_message_changed,
				applet, NULL, NULL);
	g_free (key);
	
	key = panel_applet_gconf_get_full_key (PANEL_APPLET (applet->parent), KEY_STARTED);
	applet->listeners [2] =
		gconf_client_notify_add (
				client, key,
				(GConfClientNotifyFunc) alarm_gconf_started_changed,
				applet, NULL, NULL);
	g_free (key);
	
	key = panel_applet_gconf_get_full_key (PANEL_APPLET (applet->parent), KEY_SHOW_LABEL);
	applet->listeners [3] =
		gconf_client_notify_add (
				client, key,
				(GConfClientNotifyFunc) alarm_gconf_show_label_changed,
				applet, NULL, NULL);
	g_free (key);
	
	key = panel_applet_gconf_get_full_key (PANEL_APPLET (applet->parent), KEY_LABEL_TYPE);
	applet->listeners [4] =
		gconf_client_notify_add (
				client, key,
				(GConfClientNotifyFunc) alarm_gconf_label_type_changed,
				applet, NULL, NULL);
	g_free (key);
	
	key = panel_applet_gconf_get_full_key (PANEL_APPLET (applet->parent), KEY_NOTIFY_TYPE);
	applet->listeners [5] =
		gconf_client_notify_add (
				client, key,
				(GConfClientNotifyFunc) alarm_gconf_notify_type_changed,
				applet, NULL, NULL);
	g_free (key);
	
	key = panel_applet_gconf_get_full_key (PANEL_APPLET (applet->parent), KEY_SOUND_FILE);
	applet->listeners [6] =
		gconf_client_notify_add (
				client, key,
				(GConfClientNotifyFunc) alarm_gconf_sound_file_changed,
				applet, NULL, NULL);
	g_free (key);
	

	key = panel_applet_gconf_get_full_key (PANEL_APPLET (applet->parent), KEY_SOUND_LOOP);
	applet->listeners [7] =
		gconf_client_notify_add (
				client, key,
				(GConfClientNotifyFunc) alarm_gconf_sound_loop_changed,
				applet, NULL, NULL);
	g_free (key);
	
	key = panel_applet_gconf_get_full_key (PANEL_APPLET (applet->parent), KEY_COMMAND);
	applet->listeners [8] =
		gconf_client_notify_add (
				client, key,
				(GConfClientNotifyFunc) alarm_gconf_command_changed,
				applet, NULL, NULL);
	g_free (key);
	
	key = panel_applet_gconf_get_full_key (PANEL_APPLET (applet->parent), KEY_NOTIFY_BUBBLE);
	applet->listeners [9] =
		gconf_client_notify_add (
				client, key,
				(GConfClientNotifyFunc) alarm_gconf_notify_bubble_change,
				applet, NULL, NULL);
	g_free (key);
	
}
