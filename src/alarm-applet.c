#include <string.h>
#include <sys/time.h>
#include <time.h>

#include <gtk/gtk.h>
#include <gnome.h>
#include <glade/glade.h>
#include <gdk/gdkkeysyms.h>
#include <gconf/gconf-client.h>
#include <gst/gst.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-application-registry.h>

#include <panel-applet.h>
#include <panel-applet-gconf.h>

#include "alarm-applet.h"


/*
 * DEFINTIIONS {{
 */

static GConfEnumStringPair label_type_enum_map [] = {
	{ LABEL_TYPE_ALARM,		"alarm-time"  },
	{ LABEL_TYPE_REMAIN,	"remaining-time"  },
	{ 0, NULL }
};

static GConfEnumStringPair notify_type_enum_map [] = {
	{ NOTIFY_SOUND,			"sound"  },
	{ NOTIFY_COMMAND,		"command" },
	{ 0, NULL }
};

static gchar *supported_sound_mime_types [] = { "audio", "video", "application/ogg", NULL };

/*
 * }} DEFINTIIONS
 */




/*
 * UTIL {{
 */

/**
 * Calculates the alarm timestamp given hour, min and secs.
 */
static time_t
get_alarm_timestamp (guint hour, guint minute, guint second)
{
	time_t now;
	struct tm *tm;
	
	time (&now);
	tm = localtime (&now);
	
	// Check if the alarm is for tomorrow
	if (hour < tm->tm_hour ||
		(hour == tm->tm_hour && minute < tm->tm_min) ||
		(hour == tm->tm_hour && minute == tm->tm_min && second < tm->tm_sec)) {
		
		g_debug("Alarm is for tomorrow.");
		tm->tm_mday++;
	}
	
	tm->tm_hour = hour;
	tm->tm_min = minute;
	tm->tm_sec = second;
	
	// DEBUG:
	char tmp[512];
	strftime (tmp, sizeof (tmp), "%c", tm);
	g_debug ("Alarm will trigger at %s", tmp);
	
	return mktime (tm);
}

static gchar *
to_basename (const gchar *filename)
{
	gint i, len;
	gchar *result;
	
	len = strlen (filename);
	// Remove extension
	for (i = len-1; i > 0; i--) {
		if (filename[i] == '.') {
			break;
		}
	}
	
	if (i == 0)
		// Extension not found
		i = len;
	
	result = g_strndup (filename, i);
	
	// Make first character Uppercase
	result[0] = g_utf8_strup (result, 1)[0];
	
	return result;
}

static AlarmListEntry *
alarm_list_entry_new (const gchar *name, const gchar *data, const gchar *icon)
{
	AlarmListEntry *entry;
	
	entry = g_new (AlarmListEntry, 1);
	
	entry->name = NULL;
	entry->data = NULL;
	entry->icon = NULL;
	
	if (name)
		entry->name = g_strdup (name);
	if (data)
		entry->data = g_strdup (data);
	if (icon)
		entry->icon = g_strdup (icon);
	
	return entry;
}

static AlarmListEntry *
alarm_list_entry_new_file (const gchar *data, GnomeVFSResult *ret, gchar **mime_ret)
{
	AlarmListEntry *entry;
	GnomeVFSResult result;
	GnomeVFSFileInfo *info;
	GtkIconTheme *theme;
	
	theme = gtk_icon_theme_get_default ();
	info = gnome_vfs_file_info_new ();
	
	result = gnome_vfs_get_file_info (data, info, 
				GNOME_VFS_FILE_INFO_GET_MIME_TYPE | GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
	
	if (ret != NULL)
		*ret = result;
	
	if (result != GNOME_VFS_OK) {
		g_critical ("Could not open uri: %s", data);
		return NULL;
	}
	
	entry = g_new (AlarmListEntry, 1);
	entry->data = g_strdup (data);
	entry->name = to_basename (info->name);
	entry->icon = gnome_icon_lookup(theme, NULL,
							 		entry->data, NULL, info,
							 		info->mime_type, 0, 0);
	
	g_debug ("alarm_file_entry_new(%s) => %s, %s, %s", data, entry->data, entry->name, entry->icon);
	
	if (mime_ret != NULL)
		*mime_ret = g_strdup (info->mime_type);
	
	gnome_vfs_file_info_unref (info);
	
	return entry;
}

static GList *
alarm_list_entry_list_new (const gchar *dir_uri, const gchar *supported_types[])
{
	GnomeVFSResult result;
	GnomeVFSFileInfoOptions options;
	GnomeVFSFileInfo *info;
	
	GtkIconTheme *theme;
	
	GList *list, *l, *flist;
	AlarmListEntry *entry;
	const gchar *mime;
	gboolean valid;
	gint i;
	
	theme = gtk_icon_theme_get_default ();
	
	
	options = GNOME_VFS_FILE_INFO_GET_MIME_TYPE | GNOME_VFS_FILE_INFO_FOLLOW_LINKS;
	
	result = gnome_vfs_directory_list_load (&list, dir_uri, options);
	
	if (result != GNOME_VFS_OK) {
		g_critical ("Could not open sounds directory: " ALARM_SOUNDS_DIR);
		return NULL;
	}
	
	g_debug ("Loading files in " ALARM_SOUNDS_DIR " ...");
	
	flist = NULL;
	
	for (l = list; l; l = l->next) {
		info = l->data;
		//g_debug ("-- %s", info->name);
		if (info->type == GNOME_VFS_FILE_TYPE_REGULAR) {
			mime = gnome_vfs_file_info_get_mime_type(info);
			//g_debug (" [ regular file: MIME: %s ]", mime);
			
			valid = TRUE;
			if (supported_types != NULL) {
				valid = FALSE;
				for (i = 0; supported_types[i] != NULL; i++) {
					if (strstr (mime, supported_types[i]) != NULL) {
						// MATCH
						//g_debug (" [ MATCH ]");
						valid = TRUE;
						break;
					}
				}
			}
			
			if (valid) {
				entry = g_new (AlarmListEntry, 1);
				entry->data = g_strdup_printf ("%s/%s", dir_uri, info->name);
				entry->name = to_basename (info->name);
				entry->icon = gnome_icon_lookup(theme, NULL,
												entry->data, NULL, info,
												mime, 0, 0);
				//g_debug ("Icon found: %s", entry->icon);
				flist = g_list_append (flist, entry);
			}
		}
	}
	
	gnome_vfs_file_info_list_free (list);
	
	return flist;
}

static void
alarm_list_entry_free (AlarmListEntry *e)
{
	g_free (e->data);
	g_free (e->name);
	g_free (e->icon);
	g_free (e);
}

static void
alarm_list_entry_list_free (GList **list)
{
	GList *l;
	AlarmListEntry *e;
	
	g_debug ("Alarm_file_entry_list_free (%p => %p)", list, *list);
	
	// Free data
	for (l = *list; l; l = l->next) {
		e = (AlarmListEntry *)l->data;
		alarm_list_entry_free (e);
	}
	
	g_list_free (*list);
	
	*list = NULL;
}

/*
 * }} UTIL
 */

/*
 * Command handling {{
 */

static void
command_run (AlarmApplet *applet) {
	GError *err = NULL;
	
	if (!g_spawn_command_line_async (applet->notify_command, &err)) {
		g_critical ("Could not launch `%s': %s", applet->notify_command, err->message);
		g_error_free (err);
	}
}

/*
 * }} Command handling
 */

/*
 * Media player {{
 */
static void
player_stop (AlarmApplet *applet)
{
	if (applet->gst_watch_id) {
		g_source_remove (applet->gst_watch_id);
		
		applet->gst_watch_id = 0;
	}
	
	if (applet->player != NULL) {
		g_debug ("Stopping player.");
		gst_element_set_state (applet->player, GST_STATE_NULL);
		g_debug ("Deleting pipeline\n");
		
		g_debug ("player is now %p", applet->player);
		gst_object_unref (GST_OBJECT (applet->player));
		
		applet->player = NULL;
	}
}

static void
player_preview_stop (AlarmApplet *applet)
{
	/*if (applet->gst_watch_id) {
		g_source_remove (applet->gst_watch_id);
		
		applet->gst_watch_id = 0;
	}*/
	
	if (applet->preview_player != NULL) {
		gst_element_set_state (applet->preview_player, GST_STATE_NULL);
		gst_object_unref (GST_OBJECT (applet->preview_player));
		applet->preview_player = NULL;
	}
	
	// Set stock
	gtk_button_set_label (GTK_BUTTON (applet->pref_notify_sound_preview), "gtk-media-play");
}

static gboolean
player_bus_check_errors (GstMessage *message, AlarmApplet *applet)
{
//	g_debug ("Got %s message\n", GST_MESSAGE_TYPE_NAME (message));
	
	switch (GST_MESSAGE_TYPE (message)) {
	case GST_MESSAGE_ERROR: {
		GError *err;
		gchar *debug;
		
		gst_message_parse_error (message, &err, &debug);
		g_debug ("GStreamer error: %s\n", err->message);
		display_error_dialog ("Could not play", err->message, GTK_WINDOW (applet->preferences_dialog));
		g_error_free (err);
		g_free (debug);
		
		return FALSE;
		break;
	}
	default:
		break;
	}
	
	// No errors
	return TRUE;
}

static gboolean
player_bus_cb (GstBus     *bus,
			 GstMessage *message,
			 AlarmApplet *applet)
{
//	g_debug ("Got %s message\n", GST_MESSAGE_TYPE_NAME (message));
	
	if (player_bus_check_errors (message, applet)) {
		if (GST_MESSAGE_TYPE (message) == GST_MESSAGE_EOS) {
			// End of Stream, do we loop?
			if (!applet->notify_sound_loop) {
				player_stop (applet);
				return FALSE;
			}
			
			// Loop
			gst_element_set_state (applet->player, GST_STATE_READY);
			gst_element_set_state (applet->player, GST_STATE_PLAYING);
		}
		
		// Keep notifying us
		return TRUE;
	}
	
	// There were errors
	player_stop (applet);
	
	return FALSE;
}

static gboolean
player_preview_bus_cb (GstBus     *bus,
		 			   GstMessage *message,
		 			   AlarmApplet *applet)
{
	// Any errors or EOS?
	if (!player_bus_check_errors (message, applet) ||
		GST_MESSAGE_TYPE (message) == GST_MESSAGE_EOS) {
		
		g_debug ("Stopping preview player.");
		
		player_preview_stop (applet);
		
		return FALSE;
	}

	// Keep notifying us
	return TRUE;
}

static GstElement *
create_player (const gchar *uri)
{
	GstElement *player, *audiosink, *videosink;
	
	// Initialize GStreamer
	gst_init (NULL, NULL);
	
	/* Set up player */
	player    = gst_element_factory_make ("playbin", "player");
	audiosink = gst_element_factory_make ("gconfaudiosink", "player-audiosink");
	videosink = gst_element_factory_make ("gconfvideosink", "player-videosink");
	
	if (!player || !audiosink || !videosink) {
		g_critical ("Could not create player");
		return NULL;
	}
	
	// Set uri and sinks
	g_object_set (player,
				  "uri", uri, 
				  "audio-sink", audiosink,
				  "video-sink", videosink,
				  NULL);
	
	return player;
}

/**
 * Play sound via gstreamer
 */
static void
player_start (AlarmApplet *applet)
{
	GstBus *bus;
	
	// Create player
	applet->player = create_player (get_sound_file (applet));
	
	// Attach bus watcher
	bus = gst_pipeline_get_bus (GST_PIPELINE (applet->player));
	applet->gst_watch_id = gst_bus_add_watch (bus, (GstBusFunc) player_bus_cb, applet);
	gst_object_unref (bus);
		
	// Start playing
	g_debug ("Playing %s...", get_sound_file (applet));
	
	gst_element_set_state (applet->player, GST_STATE_PLAYING);
}

static void
player_preview_start (AlarmApplet *applet)
{
	GstBus *bus;
	
	if (applet->preview_player != NULL)
		return;
	
	g_debug ("preview_start...");
	
	// Create player
	applet->preview_player = create_player (get_sound_file (applet));
	
	// Attach bus watcher
	bus = gst_pipeline_get_bus (GST_PIPELINE (applet->preview_player));
	gst_bus_add_watch (bus, (GstBusFunc) player_preview_bus_cb, applet);
	gst_object_unref (bus);
	
	gst_element_set_state (applet->preview_player, GST_STATE_PLAYING);
	
	// Set stock
	gtk_button_set_label (GTK_BUTTON (applet->pref_notify_sound_preview), "gtk-media-stop");
}

/*
 * }} Media player
 */




/*
 * GCONF SETTERS {{
 */

/**
 * Stores the alarm timestamp in gconf
 */
static void
set_alarm_time (AlarmApplet *applet, guint hour, guint minute, guint second)
{
	// Get timestamp
	time_t timestamp = get_alarm_timestamp(hour, minute, second);
	
	// Store in GConf
	panel_applet_gconf_set_int (applet->parent, KEY_ALARMTIME, (gint)timestamp, NULL);	
}

/**
 * Stores the alarm message in gconf
 */
static void
set_alarm_message (AlarmApplet *applet, const gchar *message)
{
	g_debug ("set_alarm_message %s", message);
	
	if (message == NULL)
		return;
	
	panel_applet_gconf_set_string (applet->parent, KEY_MESSAGE, message, NULL);
}

/**
 * Sets started status of alarm in gconf
 */
static void
set_alarm_started (AlarmApplet *applet, gboolean started)
{
	// Store in GConf
	panel_applet_gconf_set_bool (applet->parent, KEY_STARTED, started, NULL);	
}

/*
 * }} GCONF SETTERS
 */





/*
 * TIMER {{
 */

static void
trigger_alarm (AlarmApplet *applet)
{
	g_debug("ALARM: %s", applet->alarm_message);
	
	set_alarm_started (applet, FALSE);
	
	applet->alarm_triggered = TRUE;
	
	switch (applet->notify_type) {
	case NOTIFY_SOUND:
		// Start sound playback
		player_start (applet);
		break;
	case NOTIFY_COMMAND:
		// Start app
		g_debug ("START APP");
		command_run (applet);
		break;
	default:
		g_debug ("NOTIFICATION TYPE Not yet implemented.");
	}
}

static void
clear_alarm (AlarmApplet *applet)
{
	set_alarm_started (applet, FALSE);
	applet->alarm_triggered = FALSE;
	
	timer_remove (applet);
	update_label (applet);
	
	// Stop playback if present.
	player_stop (applet);
}

static gboolean
timer_update (AlarmApplet *applet)
{
	time_t now;
	
	time (&now);
	
	if (now >= applet->alarm_time) {
		trigger_alarm (applet);
		
		// Remove callback
		return FALSE;
	} else if (applet->alarm_time - now <= 10) {
		g_debug ("%2d...", (int)(applet->alarm_time - now));
	}
	
	if (applet->show_label && applet->label_type == LABEL_TYPE_REMAIN) {
		update_label (applet);
	}
	
	// Keep callback
	return TRUE;
}

static void
timer_start (AlarmApplet *applet)
{
	g_debug ("timer_start");
	
	// Remove old timer, if any
	timer_remove (applet);
	
	applet->timer_id = g_timeout_add_seconds (1, (GSourceFunc) timer_update, applet);
}

static void
timer_remove (AlarmApplet *applet)
{
	if (applet->timer_id > 0) {
		g_source_remove (applet->timer_id);
		
		applet->timer_id = 0;
	}
}

/*
 * }} TIMER
 */






/*
 * UI {{
 */

static void
display_error_dialog (const gchar *message, const gchar *secondary_text, GtkWindow *parent)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (parent,
									GTK_DIALOG_DESTROY_WITH_PARENT,
									GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
									message);
	
	if (secondary_text != NULL) {
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
												  secondary_text);
	}
	
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

static void
update_label (AlarmApplet *applet)
{
	gchar *tmp;
	struct tm *tm;
	guint hour, min, sec, now;
	
	if (applet->started) {
		if (applet->label_type == LABEL_TYPE_REMAIN) {
			now = time (NULL);
			sec = applet->alarm_time - now;
			
			min = sec / 60;
			sec -= min * 60;
			
			hour = min / 60;
			min -= hour * 60;
			
			tmp = g_strdup_printf(_("%02d:%02d:%02d"), hour, min, sec);
			
		} else {
			tm = localtime (&(applet->alarm_time));
			tmp = g_strdup_printf(_("%02d:%02d:%02d"), tm->tm_hour, tm->tm_min, tm->tm_sec);
		}
		
		g_object_set(applet->label, "label", tmp, NULL);
		g_free(tmp);
	} else if (applet->alarm_triggered) {
		g_object_set(applet->label, "label", _("Alarm!"), NULL);
	} else {
		g_object_set(applet->label, "label", _("No alarm"), NULL);
	}
}

static gboolean
is_separator (GtkTreeModel *model, GtkTreeIter *iter, gpointer sep_index)
{
	GtkTreePath *path;
	gboolean result;

	path = gtk_tree_model_get_path (model, iter);
	result = gtk_tree_path_get_indices (path)[0] == GPOINTER_TO_INT (sep_index);
	gtk_tree_path_free (path);

	return result;
}

/*
 * Shamelessly stolen from gnome-da-capplet.c
 */
static void
fill_combo_box (GtkComboBox *combo_box, GList *list, const gchar *custom_label)
{
	GList *l;
	GtkTreeModel *model;
	GtkCellRenderer *renderer;
	GtkTreeIter iter;
	GdkPixbuf *pixbuf = NULL;
	GtkIconTheme *theme;
	AlarmListEntry *entry;
	
	g_debug ("fill_combo_box... %d", g_list_length (list));

//	if (theme == NULL) {
	theme = gtk_icon_theme_get_default ();
//	}

	gtk_combo_box_set_row_separator_func (combo_box, is_separator,
					  GINT_TO_POINTER (g_list_length (list)), NULL);

	model = GTK_TREE_MODEL (gtk_list_store_new (2, GDK_TYPE_PIXBUF, G_TYPE_STRING));
	gtk_combo_box_set_model (combo_box, model);

	renderer = gtk_cell_renderer_pixbuf_new ();

	/* not all cells have a pixbuf, this prevents the combo box to shrink */
	gtk_cell_renderer_set_fixed_size (renderer, -1, 22);
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), renderer, FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), renderer,
					"pixbuf", PIXBUF_COL,
					NULL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), renderer,
					"text", TEXT_COL,
					NULL);

	for (l = list; l != NULL; l = g_list_next (l)) {
		entry = (AlarmListEntry *) l->data;
		
		g_debug ("Entry is %p", entry);
		g_debug ("Adding URL %s", entry->data);
		g_debug ("\t name: %s", entry->name);
		g_debug ("\t icon: %s", entry->icon);
		
		pixbuf = gtk_icon_theme_load_icon (theme, entry->icon, 22, 0, NULL);
		
		g_debug ("\t pixbuf: %s", pixbuf);
		
		g_debug ("#1");
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		g_debug ("#2");
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
					PIXBUF_COL, pixbuf,
					TEXT_COL, entry->name,
					-1);
		g_debug ("#3");
	
		//item->icon_path = gtk_tree_model_get_string_from_iter (model, &iter);
	
		if (pixbuf)
			g_object_unref (pixbuf);
	}

	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, -1);
	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			PIXBUF_COL, NULL,
			TEXT_COL, custom_label,
			-1);
}

static void
set_alarm_dialog_response_cb (GtkDialog *dialog,
							  gint rid,
							  AlarmApplet *applet)
{
	guint hour, minute, second;
	gchar *message;
	
	g_debug ("Set-Alarm Response: %s", (rid == GTK_RESPONSE_OK) ? "OK" : "Cancel");
	
	if (rid == GTK_RESPONSE_OK) {
		// Store info & start timer
		hour    = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (applet->hour));
		minute  = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (applet->minute));
		second  = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (applet->second));
		
		g_object_get (applet->message, "text", &message, NULL);
		
		set_alarm_started (applet, TRUE);
		set_alarm_time (applet, hour, minute, second);
		set_alarm_message (applet, message);
		
		g_free (message);
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	applet->set_alarm_dialog = NULL;
}

static void
set_alarm_dialog_populate (AlarmApplet *applet)
{
	if (applet->set_alarm_dialog == NULL)
		return;
	
	struct tm *tm;
	
	/* Fill fields */
	if (applet->alarm_time != 0) {
		tm = localtime (&(applet->alarm_time));
		g_object_set (applet->hour, "value", (gdouble)tm->tm_hour, NULL);
		g_object_set (applet->minute, "value", (gdouble)tm->tm_min, NULL);
		g_object_set (applet->second, "value", (gdouble)tm->tm_sec, NULL);
	}
	
	if (applet->alarm_message != NULL) {
		g_object_set (applet->message, "text", applet->alarm_message, NULL);
	}
}

static void
display_set_alarm_dialog (AlarmApplet *applet)
{
	if (applet->set_alarm_dialog != NULL) {
		// Dialog already open.
		gtk_window_present (GTK_WINDOW (applet->set_alarm_dialog));
		return;
	}
	
	GladeXML *ui = glade_xml_new(ALARM_UI_XML, "set-alarm", NULL);
	GtkWidget *ok_button;
	
	/* Fetch widgets */
	applet->set_alarm_dialog = GTK_DIALOG (glade_xml_get_widget (ui, "set-alarm"));
	applet->hour  	= glade_xml_get_widget (ui, "hour");
	applet->minute	= glade_xml_get_widget (ui, "minute");
	applet->second	= glade_xml_get_widget (ui, "second");
	applet->message = glade_xml_get_widget (ui, "message");
	ok_button = glade_xml_get_widget (ui, "ok-button");
	
	set_alarm_dialog_populate (applet);
	
	/* Set response and connect signal handlers */
	/* TODO: Fix, this isn't working */
	gtk_widget_grab_default (ok_button);
	//gtk_dialog_set_default_response (applet->set_alarm_dialog, GTK_RESPONSE_OK);
	
	g_signal_connect (applet->set_alarm_dialog, "response", 
					  G_CALLBACK (set_alarm_dialog_response_cb), applet);
}

// Load stock sounds into list
static void
load_stock_sounds_list (AlarmApplet *applet)
{
	AlarmListEntry *entry, *new_entry;
	GList *new, *l;
	guint i, new_stock_len;
	
	//if (applet->sounds != NULL)
	//	alarm_file_entry_list_free (&(applet->sounds));
	
	new = alarm_list_entry_list_new ("file://" ALARM_SOUNDS_DIR,
									 supported_sound_mime_types);
	
	new_stock_len = g_list_length (new);
	
	// Add custom entries if present
	if (g_list_length (applet->sounds) > applet->stock_sounds) {
		l = g_list_nth (applet->sounds, applet->stock_sounds);
		for (; l; l = l->next) {
			entry	  = (AlarmListEntry *) l->data;
			new_entry = alarm_list_entry_new (entry->name, entry->data, entry->icon);
			new = g_list_append (new, new_entry);
		}
	}
	
	alarm_list_entry_list_free(&(applet->sounds));
	
	// Free old stock sounds
	/*for (l = applet->sounds, i = 0; l && i < applet->stock_sounds; l = l->next, i++) {
		alarm_file_entry_free ((AlarmFileEntry *)l->data);
	}
	g_list_free (applet->sounds);*/
	
	// Update stock length and sounds
	applet->sounds = new;
	applet->stock_sounds = new_stock_len;
}

// Load stock apps into list
static void
load_app_list (AlarmApplet *applet)
{
	AlarmListEntry *entry;
	
	if (applet->apps != NULL)
		alarm_list_entry_list_free (&(applet->apps));
	
	entry = alarm_list_entry_new("Rhythmbox Music Player", "rhythmbox", "rhythmbox");
	applet->apps = g_list_append (applet->apps, entry);
}


static void
preferences_dialog_response_cb (GtkDialog *dialog,
								gint rid,
								AlarmApplet *applet)
{	
	g_debug ("Preferences Response: %s", (rid == GTK_RESPONSE_CLOSE) ? "Close" : "Unknown");
	
	// Store info & start timer
	/*hour   = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (applet->hour));
	minute = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (applet->minute));
	second = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (applet->second));
	
	set_alarm_time (applet, hour, minute, second);
	
	// Store message
	g_object_get (applet->message, "text", &message, NULL);
	g_debug ("response message %p", message);
	set_alarm_message (applet, message);
	g_free (message);
	
	update_label (applet);
	timer_start (applet);*/
	
	player_preview_stop (applet);
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	applet->preferences_dialog = NULL;
}

static void
pref_update_label_show (AlarmApplet *applet)
{
	g_object_set (applet->pref_label_show, "active", applet->show_label, NULL);
	g_object_set (applet->pref_label_type_box, "sensitive", applet->show_label, NULL);
}

static void
pref_update_label_type (AlarmApplet *applet)
{
	switch (applet->label_type) {
	case LABEL_TYPE_REMAIN:
		g_object_set (applet->pref_label_type_remaining, "active", TRUE, NULL);
		break;
	default:
		// LABEL_TYPE_ALARM
		g_object_set (applet->pref_label_type_alarm, "active", TRUE, NULL);
		break;
	}
}

static void
pref_update_notify_type (AlarmApplet *applet)
{
	// Enable selected
	switch (applet->notify_type) {
	case NOTIFY_COMMAND:
		g_object_set (applet->pref_notify_app, "active", TRUE, NULL);
		g_object_set (applet->pref_notify_app_box, "sensitive", TRUE, NULL);
		
		// Disable others
		g_object_set (applet->pref_notify_sound_box, "sensitive", FALSE, NULL);
		break;
	default:
		// NOTIFY_SOUND
		g_object_set (applet->pref_notify_sound, "active", TRUE, NULL);
		g_object_set (applet->pref_notify_sound_box, "sensitive", TRUE, NULL);
		
		// Disable others
		g_object_set (applet->pref_notify_app_box, "sensitive", FALSE, NULL);
		break;
	}
}

static void
pref_update_sound_file (AlarmApplet *applet)
{
	AlarmListEntry *item;
	GtkTreeModel *model = NULL;
	GtkTreeIter iter;
	GtkTreePath *path;
	GtkIconTheme *theme;
	GdkPixbuf *pixbuf;
	GList *l;
	guint pos, len, combo_len;
	
	/*if (applet->notify_sound_file == NULL) {
		g_debug ("pref_update_sound_file IS NULL or EMPTY");
		// Select first in list
		gtk_combo_box_set_active (GTK_COMBO_BOX (applet->pref_notify_sound_combo), 0);
		return;
	}*/
	
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (applet->pref_notify_sound_combo));
	combo_len = gtk_tree_model_iter_n_children (model, NULL);
	len = g_list_length (applet->sounds);
	
	pos = len;
	
	// Check if our list in model is up-to-date
	// We will only detect changes in size
	// Separator + Select sound file ... takes 2 spots
	if (len > combo_len - 2) {
		// Add new items
		theme = gtk_icon_theme_get_default ();
		pos = combo_len - 2;
		
		for (l = g_list_nth (applet->sounds, pos); l != NULL; l = l->next, pos++) {
			item = (AlarmListEntry *) l->data;
			
			g_debug ("Adding %s to combo...", item->name);
			
			pixbuf = gtk_icon_theme_load_icon (theme, item->icon, 22, 0, NULL);
			
			gtk_list_store_insert (GTK_LIST_STORE (model), &iter, pos);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter,
								PIXBUF_COL, pixbuf,
								TEXT_COL, item->name,
								-1);
			
			//item->icon_path = gtk_tree_model_get_string_from_iter (model, &iter);
		
			if (pixbuf)
				g_object_unref (pixbuf);
		}
	}
	
	gtk_combo_box_set_active (GTK_COMBO_BOX (applet->pref_notify_sound_combo), applet->sound_pos);
	
	g_debug ("set_row_sep_func to %d", pos);
	gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (applet->pref_notify_sound_combo),
										  is_separator, GINT_TO_POINTER (pos), NULL);
}

static void
pref_update_sound_loop (AlarmApplet *applet)
{
	g_object_set (applet->pref_notify_sound_loop, "active", applet->notify_sound_loop, NULL);
}

static void
pref_update_command (AlarmApplet *applet)
{
	g_object_set (applet->pref_notify_app_command_entry, "text", applet->notify_command, NULL);
}

static void
pref_update_show_bubble (AlarmApplet *applet)
{
	g_object_set (applet->pref_notify_bubble, "active", applet->notify_bubble, NULL);
}

static void
preferences_dialog_display (AlarmApplet *applet)
{
	if (applet->preferences_dialog != NULL) {
		// Dialog already open.
		gtk_window_present (GTK_WINDOW (applet->preferences_dialog));
		return;
	}
	
	GladeXML *ui = glade_xml_new(ALARM_UI_XML, "preferences", NULL);
	
	// Fetch widgets
	applet->preferences_dialog            = GTK_DIALOG (glade_xml_get_widget (ui, "preferences"));
	applet->pref_label_show               = glade_xml_get_widget (ui, "show-label");
	applet->pref_label_type_box           = glade_xml_get_widget (ui, "label-type-box");
	applet->pref_label_type_alarm         = glade_xml_get_widget (ui, "alarm-time");
	applet->pref_label_type_remaining     = glade_xml_get_widget (ui, "remain-time");
	applet->pref_notify_sound             = glade_xml_get_widget (ui, "play-sound");
	applet->pref_notify_sound_box         = glade_xml_get_widget (ui, "play-sound-box");
	applet->pref_notify_sound_combo       = glade_xml_get_widget (ui, "sound-combo");
	applet->pref_notify_sound_preview     = glade_xml_get_widget (ui, "sound-play");
	applet->pref_notify_sound_loop        = glade_xml_get_widget (ui, "sound-loop");
	applet->pref_notify_app               = glade_xml_get_widget (ui, "start-app");
	applet->pref_notify_app_box           = glade_xml_get_widget (ui, "app-box");
	applet->pref_notify_app_combo         = glade_xml_get_widget (ui, "app-combo");
	applet->pref_notify_app_command_box   = glade_xml_get_widget (ui, "app-command-box");
	applet->pref_notify_app_command_entry = glade_xml_get_widget (ui, "app-command");
	applet->pref_notify_bubble            = glade_xml_get_widget (ui, "notify-bubble");
	
	// Load sounds list
	load_stock_sounds_list (applet);
	
	// Fill stock sounds
	fill_combo_box (GTK_COMBO_BOX (applet->pref_notify_sound_combo),
					applet->sounds, _("Select sound file..."));
	
	// Load applications list
	load_app_list (applet);
	
	// Fill apps
	fill_combo_box (GTK_COMBO_BOX (applet->pref_notify_app_combo),
					applet->apps, _("Custom command..."));
	
	// Update UI
	pref_update_label_show (applet);
	pref_update_label_type (applet);
	pref_update_notify_type (applet);
	pref_update_sound_file (applet);
	pref_update_sound_loop (applet);
	pref_update_command (applet);
	pref_update_show_bubble (applet);
	
	// Set response and connect signal handlers
	
	g_signal_connect (applet->preferences_dialog, "response", 
					  G_CALLBACK (preferences_dialog_response_cb), applet);
	
	// Display
	g_signal_connect (applet->pref_label_show, "toggled",
					  G_CALLBACK (pref_label_show_changed_cb), applet);
	
	g_signal_connect (applet->pref_label_type_alarm, "toggled",
					  G_CALLBACK (pref_label_type_changed_cb), applet);
	
	g_signal_connect (applet->pref_label_type_remaining, "toggled",
					  G_CALLBACK (pref_label_type_changed_cb), applet);
	
	// Notification
	g_signal_connect (applet->pref_notify_sound, "toggled",
					  G_CALLBACK (pref_notify_type_changed_cb), applet);
	
	g_signal_connect (applet->pref_notify_app, "toggled",
					  G_CALLBACK (pref_notify_type_changed_cb), applet);
	
	g_signal_connect (applet->pref_notify_sound_combo, "changed",
					  G_CALLBACK (pref_notify_sound_combo_changed_cb), applet);
	
	g_signal_connect (applet->pref_notify_sound_preview, "clicked",
					  G_CALLBACK (pref_play_sound_cb), applet);
	
	g_signal_connect (applet->pref_notify_sound_loop, "toggled",
					  G_CALLBACK (pref_notify_sound_loop_changed_cb), applet);
	
	g_signal_connect (applet->pref_notify_app_combo, "changed",
					  G_CALLBACK (pref_notify_app_changed_cb), applet);
	
	g_signal_connect (applet->pref_notify_app_command_entry, "changed",
					  G_CALLBACK (pref_notify_app_command_changed_cb), applet);
	
	// Bubble
	g_signal_connect (applet->pref_notify_bubble, "toggled",
					  G_CALLBACK (pref_notify_bubble_changed_cb), applet);
	
	// Show it all
	gtk_widget_show_all (GTK_WIDGET (applet->preferences_dialog));
}

/*
 * }} UI
 */






/*
 * GCONF CALLBACKS {{
 */

static void
alarmtime_gconf_changed (GConfClient  *client,
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

static void
message_gconf_changed (GConfClient  *client,
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


static void
started_gconf_changed (GConfClient  *client,
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

static void
show_label_gconf_changed (GConfClient  *client,
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

static void
label_type_gconf_changed (GConfClient  *client,
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

static void
notify_type_gconf_changed (GConfClient  *client,
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

static gboolean
set_sound_file (AlarmApplet *applet, const gchar *uri)
{
	AlarmListEntry *item, *p;
	GnomeVFSResult result;
	gchar *mime;
	GList *l;
	guint pos, i;
	gboolean valid;
	
	if (uri == NULL || g_utf8_strlen(uri, 1) == 0)
		return FALSE;
	
	g_debug ("set_sound_file (%s)", uri);
	
	item = alarm_list_entry_new_file (uri, &result, &mime);
	
	if (item == NULL) {
		g_debug ("Invalid sound file: %s", uri);
		
		if (applet->preferences_dialog != NULL) {
			gchar *message = g_strdup_printf ("Unable to open '%s': %s.", uri, gnome_vfs_result_to_string (result));
			display_error_dialog("Invalid Sound File", message, GTK_WINDOW (applet->preferences_dialog));
			g_free(message);
		}
		
		g_free(mime);
		return FALSE;
	}
	
	// TODO: Add MIME type checks?
	valid = FALSE;
	for (i = 0; supported_sound_mime_types[i] != NULL; i++) {
		if (strstr (mime, supported_sound_mime_types[i]) != NULL) {
			// MATCH
			//g_debug (" [ MATCH ]");
			valid = TRUE;
			break;
		}
	}
				
	if (!valid) {
		g_critical ("Invalid sound file '%s': MIME-type not supported: %s", uri, mime);
		
		if (applet->preferences_dialog != NULL) {
			gchar *message = g_strdup_printf ("The file '%s' is not a valid media file.", uri);
			display_error_dialog("Invalid Sound File", message, GTK_WINDOW (applet->preferences_dialog));
			g_free(message);
		}
		
		g_free (mime);
		
		return FALSE;
	}
	
	g_free (mime);
	
	// See if it's one of the already loaded files
	for (l = applet->sounds, pos = 0; l != NULL; l = l->next, pos++) {
		p = (AlarmListEntry *) l->data;
		g_debug ("CHECK %s == %s", p->data, uri);
		if (strcmp (p->data, uri) == 0) {
			g_debug ("set_sound_file FOUND at pos %d", pos);
			break;
		}
	}
	
	g_debug ("After search, pos = %d vs length %d", pos, g_list_length (applet->sounds));
	
	// If not, add it to the list
	if (pos == g_list_length (applet->sounds)) {
		g_debug ("Adding to list: %s %s %s", item->name, item->data, item->icon);
		applet->sounds = g_list_append (applet->sounds, item);
	}
	
	applet->sound_pos = pos;
	
	return TRUE;
}

static const gchar *
get_sound_file (AlarmApplet *applet)
{
	AlarmListEntry *e;
	e = (AlarmListEntry *) g_list_nth_data (applet->sounds, applet->sound_pos);
	
	return (const gchar *)e->data;
}

static void
sound_file_gconf_changed (GConfClient  *client,
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

static void
sound_loop_gconf_changed (GConfClient  *client,
						  guint         cnxn_id,
						  GConfEntry   *entry,
						  AlarmApplet  *applet)
{
	g_debug ("sound_loop_changed");
	
	if (!entry->value || entry->value->type != GCONF_VALUE_BOOL)
		return;
	
	applet->notify_sound_loop = gconf_value_get_bool (entry->value);
	
	if (applet->preferences_dialog != NULL) {
		pref_update_sound_loop (applet);
	}
}



static void
command_gconf_changed (GConfClient  *client,
					   guint         cnxn_id,
					   GConfEntry   *entry,
					   AlarmApplet  *applet)
{
	g_debug ("command_changed");
	
	if (!entry->value || entry->value->type != GCONF_VALUE_STRING)
		return;
	
	applet->notify_command = (gchar *)gconf_value_get_string (entry->value);
	
	if (applet->preferences_dialog != NULL) {
		pref_update_command (applet);
	}
}

static void
notify_bubble_gconf_changed (GConfClient  *client,
							 guint         cnxn_id,
							 GConfEntry   *entry,
							 AlarmApplet  *applet)
{
	g_debug ("notify_bubble_changed");
	
	if (!entry->value || entry->value->type != GCONF_VALUE_BOOL)
		return;
	
	applet->notify_bubble = gconf_value_get_bool (entry->value);
	
	if (applet->preferences_dialog != NULL) {
		pref_update_show_bubble (applet);
	}
}


/*
 * }} GCONF CALLBACKS
 */

/*
 * UI CALLBACKS {{
 */

static void
pref_label_show_changed_cb (GtkToggleButton *togglebutton,
							AlarmApplet *applet)
{
	gboolean value = gtk_toggle_button_get_active (togglebutton);
	
	panel_applet_gconf_set_bool (applet->parent, KEY_SHOW_LABEL, value, NULL);
}

static void
pref_label_type_changed_cb (GtkToggleButton *togglebutton,
							AlarmApplet *applet)
{
	const gchar *name = gtk_widget_get_name (GTK_WIDGET (togglebutton));
	gboolean value    = gtk_toggle_button_get_active (togglebutton);
	const gchar *kval;
	
	if (!value) {
		// Not checked, not interested
		return;
	}
	
	if (strcmp (name, "remain-time") == 0) {
		kval = gconf_enum_to_string (label_type_enum_map, LABEL_TYPE_REMAIN);
		panel_applet_gconf_set_string (applet->parent, KEY_LABEL_TYPE, kval, NULL);
	} else {
		kval = gconf_enum_to_string (label_type_enum_map, LABEL_TYPE_ALARM);
		panel_applet_gconf_set_string (applet->parent, KEY_LABEL_TYPE, kval, NULL);
	}
}

static void
pref_notify_type_changed_cb (GtkToggleButton *togglebutton,
							AlarmApplet *applet)
{
	const gchar *name = gtk_widget_get_name (GTK_WIDGET (togglebutton));
	gboolean value    = gtk_toggle_button_get_active (togglebutton);
	const gchar *kval;
	
	if (!value) {
		// Not checked, not interested
		return;
	}
	
	if (strcmp (name, "start-app") == 0) {
		kval = gconf_enum_to_string (notify_type_enum_map, NOTIFY_COMMAND);
		panel_applet_gconf_set_string (applet->parent, KEY_NOTIFY_TYPE, kval, NULL);
	} else {
		kval = gconf_enum_to_string (notify_type_enum_map, NOTIFY_SOUND);
		panel_applet_gconf_set_string (applet->parent, KEY_NOTIFY_TYPE, kval, NULL);
	}
}

static void
pref_open_sound_file_chooser (AlarmApplet *applet)
{
	GtkWidget *dialog;
	
	dialog = gtk_file_chooser_dialog_new (_("Select sound file..."),
					      GTK_WINDOW (applet->preferences_dialog),
					      GTK_FILE_CHOOSER_ACTION_OPEN,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					      NULL);
	
	gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (dialog), get_sound_file (applet));
	
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		gchar *uri;
		
		uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));
		
		g_debug ("RESPONSE ACCEPT: %s", uri);
		panel_applet_gconf_set_string (applet->parent, KEY_SOUND_FILE, uri, NULL);
		
		g_free (uri);
	} else {
		g_debug ("RESPONSE CANCEL");
		pref_update_sound_file (applet);
	}
	
	gtk_widget_destroy (dialog);
}

static void
pref_notify_sound_combo_changed_cb (GtkComboBox *combo,
									AlarmApplet *applet)
{
	g_debug ("Combo_changed");
	
	GtkTreeModel *model;
	AlarmListEntry *item;
	guint current_index, len, combo_len;
	
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (applet->pref_notify_sound_combo));
	combo_len = gtk_tree_model_iter_n_children (model, NULL);
	current_index = gtk_combo_box_get_active (combo);
	len = g_list_length (applet->sounds);
	
	g_debug ("Current index: %d, n sounds: %d, sound index: %d", current_index, len, applet->sound_pos);
	
	if (current_index < 0)
		// None selected
		return;
	
	if (current_index >= len) {
		// Select sound file
		g_debug ("Open SOUND file chooser...");
		pref_open_sound_file_chooser (applet);
		return;
	}
	
	item = (AlarmListEntry *) g_list_nth_data (applet->sounds, current_index);
	
	panel_applet_gconf_set_string (applet->parent, KEY_SOUND_FILE, item->data, NULL);
}

static void
pref_notify_sound_loop_changed_cb (GtkToggleButton *togglebutton,
							AlarmApplet *applet)
{
	gboolean value = gtk_toggle_button_get_active (togglebutton);
	
	panel_applet_gconf_set_bool (applet->parent, KEY_SOUND_LOOP, value, NULL);
}

static void
pref_play_sound_cb (GtkButton *button,
					AlarmApplet *applet)
{
	if (applet->preview_player != NULL) {
		// Stop preview player
		player_preview_stop (applet);
	} else {
		// Start preview player
		player_preview_start (applet);
	}
}

static void
pref_notify_app_changed_cb (GtkEditable *editable,
							AlarmApplet *applet)
{
	
	
}

static void
pref_notify_app_command_changed_cb (GtkEditable *editable,
								AlarmApplet *applet)
{
	const gchar *value = gtk_entry_get_text (GTK_ENTRY (editable));
	
	panel_applet_gconf_set_string (applet->parent, KEY_COMMAND, value, NULL);
}

static void
pref_notify_bubble_changed_cb (GtkToggleButton *togglebutton,
							AlarmApplet *applet)
{
	gboolean value = gtk_toggle_button_get_active (togglebutton);
	
	panel_applet_gconf_set_bool (applet->parent, KEY_NOTIFY_BUBBLE, value, NULL);
}

static void
menu_set_alarm_cb (BonoboUIComponent *component,
				   gpointer data,
				   const gchar *cname)
{
	AlarmApplet *applet = (AlarmApplet *)data;
	display_set_alarm_dialog (applet);
}

static void
menu_clear_alarm_cb (BonoboUIComponent *component,
					 gpointer data,
					 const gchar *cname)
{
	AlarmApplet *applet = (AlarmApplet *)data;
	
	clear_alarm (applet);
	
	g_debug("alarm_applet_clear_alarm");
}

static void
menu_preferences_cb (BonoboUIComponent *component,
								gpointer data,
								const gchar *cname)
{
	/* Construct the preferences dialog and show it here */
	g_debug("preferences_dialog");
	
	AlarmApplet *applet = (AlarmApplet *)data;
	
	preferences_dialog_display (applet);
}

static void
menu_about_cb (BonoboUIComponent *component,
			   gpointer data,
			   const gchar *cname)
{
	/* Construct the about dialog and show it here */
	g_debug("about_dialog");
}

static gboolean
button_cb (GtkWidget *widget,
						GdkEventButton *event,
						gpointer data)
{
	AlarmApplet *applet = (AlarmApplet *)data;
	
	g_debug("BUTTON: %d", event->button);
	
	/* React only to left mouse button */
	if (event->button == 2 || event->button == 3) {
		return FALSE;
	}
	
	/* TODO: if alarm is triggered { snooze } else */
	
	if (applet->alarm_triggered) {
		g_debug ("Stopping alarm!");
		clear_alarm (applet);
	} else {
		display_set_alarm_dialog (applet);
	}
	
	return TRUE;
}

static void
destroy_cb (GtkObject *object, AlarmApplet *applet)
{
	if (applet->sounds != NULL) {
		alarm_list_entry_list_free(&(applet->sounds));
	}

	timer_remove (applet);
}

/*
 * }} UI CALLBACKS
 */





/*
 * INIT {{
 */

static void
menu_setup (AlarmApplet *applet)
{
	static const gchar *menu_xml =
		"<popup name=\"button3\">\n"
		"   <menuitem name=\"Set Alarm Item\" "
		"			  verb=\"SetAlarm\" "
		"			_label=\"_Set Alarm\" "
		"		   pixtype=\"stock\" "
		"		   pixname=\"gtk-apply\"/>\n"
		"   <menuitem name=\"Clear Item\" "
		"			  verb=\"ClearAlarm\" "
		"			_label=\"_Clear alarm\" "
		"		   pixtype=\"stock\" "
		"		   pixname=\"gtk-clear\"/>\n"
		"   <separator/>\n"
		"   <menuitem name=\"Preferences Item\" "
		"             verb=\"Preferences\" "
		"           _label=\"_Preferences...\"\n"
		"          pixtype=\"stock\" "
		"          pixname=\"gtk-properties\"/>\n"
		"   <menuitem name=\"About Item\" "
		"             verb=\"About\" "
		"           _label=\"_About...\"\n"
		"          pixtype=\"stock\" "
		"          pixname=\"gtk-about\"/>\n"
		"</popup>\n";
	
	static const BonoboUIVerb menu_verbs [] = {
			BONOBO_UI_VERB ("SetAlarm", menu_set_alarm_cb),
			BONOBO_UI_VERB ("ClearAlarm", menu_clear_alarm_cb),
			BONOBO_UI_VERB ("Preferences", menu_preferences_cb),
			BONOBO_UI_VERB ("About", menu_about_cb),
			BONOBO_UI_VERB_END
	};
	
	panel_applet_setup_menu (PANEL_APPLET (applet->parent),
	                         menu_xml,
	                         menu_verbs,
	                         applet);
}

static void
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
				(GConfClientNotifyFunc) alarmtime_gconf_changed,
				applet, NULL, NULL);
	g_free (key);
	
	key = panel_applet_gconf_get_full_key (PANEL_APPLET (applet->parent), KEY_MESSAGE);
	applet->listeners [1] =
		gconf_client_notify_add (
				client, key,
				(GConfClientNotifyFunc) message_gconf_changed,
				applet, NULL, NULL);
	g_free (key);
	
	key = panel_applet_gconf_get_full_key (PANEL_APPLET (applet->parent), KEY_STARTED);
	applet->listeners [2] =
		gconf_client_notify_add (
				client, key,
				(GConfClientNotifyFunc) started_gconf_changed,
				applet, NULL, NULL);
	g_free (key);
	
	key = panel_applet_gconf_get_full_key (PANEL_APPLET (applet->parent), KEY_SHOW_LABEL);
	applet->listeners [3] =
		gconf_client_notify_add (
				client, key,
				(GConfClientNotifyFunc) show_label_gconf_changed,
				applet, NULL, NULL);
	g_free (key);
	
	key = panel_applet_gconf_get_full_key (PANEL_APPLET (applet->parent), KEY_LABEL_TYPE);
	applet->listeners [4] =
		gconf_client_notify_add (
				client, key,
				(GConfClientNotifyFunc) label_type_gconf_changed,
				applet, NULL, NULL);
	g_free (key);
	
	key = panel_applet_gconf_get_full_key (PANEL_APPLET (applet->parent), KEY_NOTIFY_TYPE);
	applet->listeners [5] =
		gconf_client_notify_add (
				client, key,
				(GConfClientNotifyFunc) notify_type_gconf_changed,
				applet, NULL, NULL);
	g_free (key);
	
	key = panel_applet_gconf_get_full_key (PANEL_APPLET (applet->parent), KEY_SOUND_FILE);
	applet->listeners [6] =
		gconf_client_notify_add (
				client, key,
				(GConfClientNotifyFunc) sound_file_gconf_changed,
				applet, NULL, NULL);
	g_free (key);
	

	key = panel_applet_gconf_get_full_key (PANEL_APPLET (applet->parent), KEY_SOUND_LOOP);
	applet->listeners [7] =
		gconf_client_notify_add (
				client, key,
				(GConfClientNotifyFunc) sound_loop_gconf_changed,
				applet, NULL, NULL);
	g_free (key);
	
	key = panel_applet_gconf_get_full_key (PANEL_APPLET (applet->parent), KEY_COMMAND);
	applet->listeners [8] =
		gconf_client_notify_add (
				client, key,
				(GConfClientNotifyFunc) command_gconf_changed,
				applet, NULL, NULL);
	g_free (key);
	
	key = panel_applet_gconf_get_full_key (PANEL_APPLET (applet->parent), KEY_NOTIFY_BUBBLE);
	applet->listeners [9] =
		gconf_client_notify_add (
				client, key,
				(GConfClientNotifyFunc) notify_bubble_gconf_changed,
				applet, NULL, NULL);
	g_free (key);
	
}

static gboolean
alarm_applet_factory (PanelApplet *panelapplet,
					  const gchar *iid,
					  gpointer data)
{
	AlarmApplet *applet;
	AlarmListEntry *item;
	GtkWidget *hbox;
	GdkPixbuf *icon;
	gchar *tmp;
	
	if (strcmp (iid, "OAFIID:GNOME_AlarmApplet") != 0)
		return FALSE;
	
	
	/* Initialize applet struct */
	applet = g_new(AlarmApplet, 1);
	applet->alarm_time = 0;
	applet->alarm_message = NULL;
	applet->started = FALSE;
	applet->alarm_triggered = FALSE;
	applet->timer_id = 0;
	applet->gst_watch_id = 0;
	applet->player = NULL;
	applet->preview_player = NULL;
	applet->sounds = NULL;
	applet->sound_pos = 0;
	applet->apps = NULL;
	
	applet->parent = panelapplet;
	
	applet->set_alarm_dialog = NULL;
	applet->preferences_dialog = NULL;
	
	/* Preferences (defaults) TODO: Add GConf schema */
	applet->show_label = TRUE;
	applet->label_type = LABEL_TYPE_ALARM;
	applet->notify_type = NOTIFY_SOUND;
	applet->notify_sound_loop = TRUE;
	applet->notify_bubble = TRUE;
	
	/* Prepare */
	panel_applet_add_preferences (applet->parent, ALARM_SCHEMA_DIR, NULL);
	panel_applet_set_flags (PANEL_APPLET (panelapplet), PANEL_APPLET_EXPAND_MINOR);
	
	/* Set up gconf handlers */
	setup_gconf (applet);
	
	/* Set up properties menu */
	menu_setup(applet);
	
	/* Set up applet */
	g_signal_connect (G_OBJECT(panelapplet), "button-press-event",
					  G_CALLBACK(button_cb), applet);
	
	g_signal_connect (G_OBJECT(panelapplet), "destroy",
					  G_CALLBACK(destroy_cb), applet);
	
	/* Set up container hbox */
	hbox = gtk_hbox_new(FALSE, 6);
	
	/* Set up icon and label */
	icon = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
									 ALARM_ICON,
									 22,
									 0, NULL);
	
	if (icon == NULL) {
		g_critical ("Icon not found.");
	}
	
	applet->icon = gtk_image_new_from_pixbuf (icon);
	
	if (icon)
		g_object_unref (icon);
	
	applet->label = g_object_new(GTK_TYPE_LABEL,
								 "label", _("No alarm"),
								 "use-markup", TRUE,
								 NULL);
	
	/* Pack */
	gtk_box_pack_start_defaults(GTK_BOX(hbox), applet->icon);
	gtk_box_pack_start_defaults(GTK_BOX(hbox), applet->label);
	
	/* Add to container and show */
	gtk_container_add (GTK_CONTAINER (applet->parent), hbox);
	gtk_widget_show_all (GTK_WIDGET (applet->parent));
	
	
	
	/* Fetch gconf data */
	applet->started       = panel_applet_gconf_get_bool (applet->parent, KEY_STARTED, NULL);
	applet->alarm_time    = panel_applet_gconf_get_int (applet->parent, KEY_ALARMTIME, NULL);
	tmp					  = panel_applet_gconf_get_string (applet->parent, KEY_MESSAGE, NULL);
	
	applet->alarm_message = g_strdup (tmp);
	
	if (applet->started) {
		// Start the timer!
		update_label (applet);
		timer_start (applet);
	}
	
	/* Fetch preference data */
	applet->show_label          = panel_applet_gconf_get_bool (applet->parent, KEY_SHOW_LABEL, NULL);
	applet->notify_sound_loop   = panel_applet_gconf_get_bool (applet->parent, KEY_SOUND_LOOP, NULL);
	applet->notify_command      = panel_applet_gconf_get_string (applet->parent, KEY_COMMAND, NULL);
	applet->notify_bubble       = panel_applet_gconf_get_bool (applet->parent, KEY_NOTIFY_BUBBLE, NULL);
	
	// Load sounds list
	load_stock_sounds_list (applet);
	
	// Fetch sound file
	tmp = panel_applet_gconf_get_string (applet->parent, KEY_SOUND_FILE, NULL);
	
	if (!set_sound_file (applet, tmp)) {
		if (g_list_length (applet->sounds) > 0) {
			// Set it to the first stock sound
			applet->sound_pos = 0;
		}
	}
	g_free(tmp);
	
	tmp = panel_applet_gconf_get_string (applet->parent, KEY_LABEL_TYPE, NULL);
	if (tmp) {
		gconf_string_to_enum (label_type_enum_map, tmp, (gint *)&(applet->label_type));
	}
	g_free(tmp);
	
	tmp = panel_applet_gconf_get_string (applet->parent, KEY_NOTIFY_TYPE, NULL);
	if (tmp) {
		gconf_string_to_enum (notify_type_enum_map, tmp, (gint *)&(applet->notify_type));
	}
	g_free(tmp);
	
	// Set label visibility
	g_object_set (applet->label, "visible", applet->show_label, NULL);
	
	g_debug ("GLADE DIR: %s", GNOME_GLADEDIR);

	return TRUE;
}

PANEL_APPLET_BONOBO_FACTORY ("OAFIID:GNOME_AlarmApplet_Factory",
                             PANEL_TYPE_APPLET,
                             "alarm_applet",
                             VERSION,
                             alarm_applet_factory,
                             NULL);


/*
 * }} INIT
 */
