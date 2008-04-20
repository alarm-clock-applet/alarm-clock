#include "edit-alarm.h"
#include "alarm-applet.h"
#include "alarm.h"
#include "player.h"

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

static void
alarm_settings_update_time (AlarmSettingsDialog *dialog);

static void
time_changed_cb (GtkSpinButton *spinbutton, gpointer data);

static void
sound_combo_changed_cb (GtkComboBox *combo, AlarmSettingsDialog *dialog);

static void
app_combo_changed_cb (GtkComboBox *combo, AlarmSettingsDialog *dialog);



#define REPEAT_LABEL	_("<b>T_rigger alarm:</b> %s")


/*
 * Utility functions
 */

static GtkWidget *
create_img_label (const gchar *label_text, const gchar *icon_name)
{
	gchar *tmp;
	
	tmp = g_strdup_printf ("<b>%s</b>", label_text);
	
	GdkPixbuf *icon;
	GtkWidget *img, *label, *spacer;	// TODO: Ugly with spacer
	GtkWidget *hbox;
	
	icon = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
									 icon_name,
									 24,
									 0, NULL);
	img = gtk_image_new_from_pixbuf(icon);
	label = g_object_new (GTK_TYPE_LABEL,
						  "label", tmp,
						  "use-markup", TRUE,
						  "xalign", 0.0,
						  NULL);
	
	hbox = g_object_new (GTK_TYPE_HBOX, "spacing", 6, NULL);
	
	spacer = g_object_new (GTK_TYPE_LABEL, "label", "", NULL);
	gtk_box_pack_start_defaults (GTK_BOX (hbox), spacer);
	
	gtk_box_pack_start(GTK_BOX (hbox), img, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX (hbox), label, FALSE, FALSE, 0);
	
	spacer = g_object_new (GTK_TYPE_LABEL, "label", "", NULL);
	gtk_box_pack_start_defaults (GTK_BOX (hbox), spacer);
	
	g_free (tmp);
	
	return hbox;
}





/*
 * Utility functions for updating various parts of the settings dialog.
 */

static void
alarm_settings_update_type (AlarmSettingsDialog *dialog)
{
	if (dialog->alarm->type == ALARM_TYPE_CLOCK) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->clock_toggle), TRUE);
		gtk_widget_set_sensitive(dialog->repeat_expand, TRUE);
	} else {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->timer_toggle), TRUE);
		gtk_widget_set_sensitive(dialog->repeat_expand, FALSE);
	}
}

static void
alarm_settings_update_time (AlarmSettingsDialog *dialog)
{
	struct tm *tm;
	
	if (dialog->alarm->type == ALARM_TYPE_CLOCK) {
		tm = localtime(&(dialog->alarm->time));
	} else {
		// TIMER
		tm = gmtime (&(dialog->alarm->timer));
	}
	
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->hour_spin), tm->tm_hour);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->min_spin), tm->tm_min);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->sec_spin), tm->tm_sec);
}

static void
alarm_settings_update_repeat (AlarmSettingsDialog *dialog)
{
	AlarmRepeat r;
	gint i;
	gboolean check;
	gchar *label;
	
	/*
	 * Update check boxes
	 */
	for (r = ALARM_REPEAT_SUN, i = 0; r <= ALARM_REPEAT_SAT; r = 1 << ++i) {
		check = (dialog->alarm->repeat & r) != 0;
		
		// Activate the appropriate widget
		g_object_set (dialog->repeat_check[i], "active", check, NULL);
	}
	
	
	/*
	 * Update fancy expander label
	 */
	
	if (dialog->alarm->repeat == ALARM_REPEAT_NONE) {
		// NO REPEAT
		label = g_strdup_printf (REPEAT_LABEL, _("Once"));
		
	} else if (dialog->alarm->repeat == ALARM_REPEAT_WEEKDAYS) {
		// REPEAT WEEKDAYS
		label = g_strdup_printf (REPEAT_LABEL, _("Weekdays"));
		
	} else if (dialog->alarm->repeat == ALARM_REPEAT_WEEKENDS) {
		// REPEAT WEEKENDS
		label = g_strdup_printf (REPEAT_LABEL, _("Weekends"));
		
	} else if (dialog->alarm->repeat == ALARM_REPEAT_ALL) {
		// REPEAT WEEK
		label = g_strdup_printf (REPEAT_LABEL, _("Every day"));
		
	} else {
		// CUSTOM
		label = g_strdup_printf (REPEAT_LABEL, _("Custom"));
	}
	
	g_object_set (dialog->repeat_label, "label", label, NULL);
	
	g_free (label);
}

static void
alarm_settings_update_notify_type (AlarmSettingsDialog *dialog)
{
	// Enable selected
	switch (dialog->alarm->notify_type) {
	case ALARM_NOTIFY_COMMAND:
		g_object_set (dialog->notify_app_radio, "active", TRUE, NULL);
		g_object_set (dialog->notify_app_box, "sensitive", TRUE, NULL);
		
		// Disable others
		g_object_set (dialog->notify_sound_box, "sensitive", FALSE, NULL);
		break;
	default:
		// NOTIFY_SOUND
		g_object_set (dialog->notify_sound_radio, "active", TRUE, NULL);
		g_object_set (dialog->notify_sound_box, "sensitive", TRUE, NULL);
		
		// Disable others
		g_object_set (dialog->notify_app_box, "sensitive", FALSE, NULL);
		break;
	}
}

static void
alarm_settings_update_sound (AlarmSettingsDialog *dialog)
{
	AlarmListEntry *item;
	GList *l;
	guint pos, len, combo_len;
	gint sound_pos = -1;
	
	g_debug ("alarm_settings_update_sound (%p): sound_combo: %p, applet: %p, sounds: %p", dialog, dialog->notify_sound_combo, dialog->applet, dialog->applet->sounds);
	
	/* Fill sounds list */
	fill_combo_box (GTK_COMBO_BOX (dialog->notify_sound_combo),
					dialog->applet->sounds, _("Select sound file..."));
	
	// Look for the selected sound file
	for (l = dialog->applet->sounds, pos = 0; l != NULL; l = l->next, pos++) {
		item = (AlarmListEntry *)l->data;
		if (strcmp (item->data, dialog->alarm->sound_file) == 0) {
			// Match!
			sound_pos = pos;
			gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->notify_sound_combo), pos);
			break;
		}
	}
}

static void
alarm_settings_update_app (AlarmSettingsDialog *dialog)
{
	AlarmListEntry *item;
	GList *l;
	guint pos, len, combo_len;
	gboolean custom = FALSE;
	
	g_debug ("alarm_settings_update_app (%p): app_combo: %p, applet: %p, apps: %p", dialog, dialog->notify_app_combo, dialog->applet, dialog->applet->apps);
	g_debug ("alarm_settings_update_app setting entry to %s", dialog->alarm->command);
	
	/* Fill apps list */
	fill_combo_box (GTK_COMBO_BOX (dialog->notify_app_combo),
					dialog->applet->apps, _("Custom command..."));
	
	// Look for the selected command
	len = g_list_length (dialog->applet->apps);
	for (l = dialog->applet->apps, pos = 0; l != NULL; l = l->next, pos++) {
		item = (AlarmListEntry *)l->data;
		if (strcmp (item->data, dialog->alarm->command) == 0) {
			// Match!
			break;
		}
	}
	
	/* Only change sensitivity of the command entry if user
	 * isn't typing a custom command there already. */ 
	if (pos >= len) {
		// Custom command
		pos += 1;
		custom = TRUE;
	}
	
	g_debug ("CMD ENTRY HAS FOCUS? %d", GTK_WIDGET_HAS_FOCUS (dialog->notify_app_command_entry));
	
	if (!GTK_WIDGET_HAS_FOCUS (dialog->notify_app_command_entry))
		g_object_set (dialog->notify_app_command_entry, "sensitive", custom, NULL);
	
	gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->notify_app_combo), pos);
}

static void
alarm_settings_update (AlarmSettingsDialog *dialog)
{
	Alarm *alarm = ALARM (dialog->alarm);
	
	g_object_set (dialog->label_entry, "text", alarm->message, NULL);
	g_object_set (dialog->notify_sound_loop_check, "active", alarm->sound_loop, NULL);
	g_object_set (dialog->notify_app_command_entry, "text", dialog->alarm->command, NULL);
	g_object_set (dialog->notify_bubble_check, "active", alarm->notify_bubble, NULL);
	
	alarm_settings_update_type (dialog);
	alarm_settings_update_time (dialog);
	alarm_settings_update_repeat (dialog);
	alarm_settings_update_notify_type (dialog);
	alarm_settings_update_sound (dialog);
	alarm_settings_update_app (dialog);
}






/*
 * Alarm object callbacks
 */

static void
alarm_type_changed (GObject *object, 
					GParamSpec *param,
					gpointer data)
{
	AlarmSettingsDialog *dialog = (AlarmSettingsDialog *)data;
	
	alarm_settings_update_type (dialog);
}

static void
alarm_time_changed (GObject *object, 
					GParamSpec *param,
					gpointer data)
{
	AlarmSettingsDialog *dialog = (AlarmSettingsDialog *)data;
	
	/* Only interesting if alarm is a CLOCK */
	if (dialog->alarm->type != ALARM_TYPE_CLOCK)
		return;
	
	alarm_settings_update_time (dialog);
}

static void
alarm_repeat_changed (GObject *object, 
					  GParamSpec *param,
					  gpointer data)
{
	AlarmSettingsDialog *dialog = (AlarmSettingsDialog *)data;
	
	alarm_settings_update_repeat (dialog);
}

static void
alarm_timer_changed (GObject *object, 
					 GParamSpec *param,
					 gpointer data)
{
	AlarmSettingsDialog *dialog = (AlarmSettingsDialog *)data;
	
	/* Only interesting if alarm is a TIMER */
	if (dialog->alarm->type != ALARM_TYPE_TIMER)
		return;
	
	alarm_settings_update_time (dialog);
}

static void
alarm_notify_type_changed (GObject *object, 
						   GParamSpec *param,
						   gpointer data)
{
	AlarmSettingsDialog *dialog = (AlarmSettingsDialog *)data;
	
	alarm_settings_update_notify_type (dialog);
}

static void
alarm_settings_sound_file_changed (GObject *object, 
								   GParamSpec *param,
								   gpointer data)
{
	AlarmSettingsDialog *dialog = (AlarmSettingsDialog *)data;
	
	g_debug ("alarm_settings_sound_file_changed (%p)", data);
	
	// Block sound combo signals to prevent infinite loop
	// because the "changed" signal will be emitted when we
	// change the combo box tree model.
	g_signal_handlers_block_matched (dialog->notify_sound_combo, 
									 G_SIGNAL_MATCH_FUNC, 
									 0, 0, NULL, sound_combo_changed_cb, NULL);
	
	// Update UI
	alarm_settings_update_sound (dialog);
	
	// Unblock combo signals
	g_signal_handlers_unblock_matched (dialog->notify_sound_combo, 
									   G_SIGNAL_MATCH_FUNC,
									   0, 0, NULL, sound_combo_changed_cb, NULL);
}

static void
alarm_sound_repeat_changed (GObject *object, 
							GParamSpec *param,
							gpointer data)
{
	AlarmSettingsDialog *dialog = (AlarmSettingsDialog *)data;
	
	g_debug ("alarm_sound_repeat_changed to: %d", dialog->alarm->sound_loop);
}

static void
alarm_settings_command_changed (GObject *object, 
								GParamSpec *param,
								gpointer data)
{
	AlarmSettingsDialog *dialog = (AlarmSettingsDialog *)data;
	
	g_debug ("alarm_settings_command_changed (%p)", data);
	
	// Block sound combo signals to prevent infinite loop
	// because the "changed" signal will be emitted when we
	// change the combo box tree model.
	g_signal_handlers_block_matched (dialog->notify_app_combo, 
									 G_SIGNAL_MATCH_FUNC, 
									 0, 0, NULL, app_combo_changed_cb, NULL);
	
	// Update UI
	alarm_settings_update_app (dialog);
	
	// Unblock combo signals
	g_signal_handlers_unblock_matched (dialog->notify_app_combo, 
									   G_SIGNAL_MATCH_FUNC,
									   0, 0, NULL, app_combo_changed_cb, NULL);
}


/*
 * GUI utils
 */

static void
open_sound_file_chooser (AlarmSettingsDialog *dialog)
{
	GtkWidget *chooser;
	
	chooser = gtk_file_chooser_dialog_new (_("Select sound file..."),
										   GTK_WINDOW (dialog->dialog),
										   GTK_FILE_CHOOSER_ACTION_OPEN,
										   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
										   GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
										   NULL);
	
	gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (chooser), dialog->alarm->sound_file);
	
	if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT) {
		gchar *uri;
		
		uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (chooser));
		
		g_debug ("RESPONSE ACCEPT: %s", uri);
		
		g_object_set (dialog->alarm, "sound_file", uri, NULL);
		
		g_free (uri);
	} else {
		g_debug ("RESPONSE CANCEL");
		alarm_settings_update_sound (dialog);
	}
	
	gtk_widget_destroy (chooser);
}



/*
 * GUI callbacks
 */

static void
type_toggle_cb (GtkToggleButton *toggle, gpointer data)
{
	AlarmSettingsDialog *dialog = (AlarmSettingsDialog *)data;
	GtkWidget *toggle2 = (GTK_WIDGET (toggle) == dialog->clock_toggle) ? dialog->timer_toggle : dialog->clock_toggle;
	gboolean toggled = gtk_toggle_button_get_active(toggle);
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle2), !toggled);
	
	if (GTK_WIDGET (toggle) == dialog->clock_toggle && toggled) {
		g_object_set (dialog->alarm, "type", ALARM_TYPE_CLOCK, NULL);
	} else {
		g_object_set (dialog->alarm, "type", ALARM_TYPE_TIMER, NULL);
	}
	
	time_changed_cb (GTK_SPIN_BUTTON (dialog->hour_spin), dialog);
}

static void
time_changed_cb (GtkSpinButton *spinbutton, gpointer data)
{
	AlarmSettingsDialog *dialog = (AlarmSettingsDialog *)data;
	guint hour, min, sec;
	
	hour = gtk_spin_button_get_value (GTK_SPIN_BUTTON (dialog->hour_spin));
	min = gtk_spin_button_get_value (GTK_SPIN_BUTTON (dialog->min_spin));
	sec = gtk_spin_button_get_value (GTK_SPIN_BUTTON (dialog->sec_spin));
	
	if (dialog->alarm->type == ALARM_TYPE_CLOCK) {
		alarm_set_time (dialog->alarm, hour, min, sec);
	} else {
		alarm_set_timer (dialog->alarm, hour, min, sec);
	}
}

static void
repeat_changed_cb (GtkToggleButton *togglebutton,
				   gpointer         data)
{
	AlarmSettingsDialog *dialog = (AlarmSettingsDialog *)data;
	
	const gchar *name;
	AlarmRepeat rep, new_rep;
	gboolean active;

	/* The check buttons have the same name as the 3 letter
	 * string representation of the day.
	 */
	name   = gtk_widget_get_name (GTK_WIDGET (togglebutton));
	rep    = alarm_repeat_from_string (name);
	active = gtk_toggle_button_get_active (togglebutton);
	
	if (active)
		// Add rep
		new_rep = dialog->alarm->repeat | rep;
	else
		// Remove rep
		new_rep = dialog->alarm->repeat & ~rep;
	
	g_object_set (dialog->alarm, "repeat", new_rep, NULL);
}

static void
notify_type_changed_cb (GtkToggleButton *togglebutton,
						AlarmSettingsDialog *dialog)
{
	const gchar *name = gtk_widget_get_name (GTK_WIDGET (togglebutton));
	gboolean value    = gtk_toggle_button_get_active (togglebutton);
	
	if (!value) {
		// Not checked, not interested
		return;
	}
	
	g_debug ("notify_type_changed: %s", name);
	
	if (strcmp (name, "app-radio") == 0) {
		g_object_set (dialog->alarm, "notify_type", ALARM_NOTIFY_COMMAND, NULL);
	} else {
		g_object_set (dialog->alarm, "notify_type", ALARM_NOTIFY_SOUND, NULL);
	}
	
	alarm_settings_update_notify_type (dialog);
}

static void
sound_combo_changed_cb (GtkComboBox *combo,
						AlarmSettingsDialog *dialog)
{
	g_debug ("SOUND Combo_changed");
	
	GtkTreeModel *model;
	AlarmListEntry *item;
	guint current_index, len, combo_len;
	
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (dialog->notify_sound_combo));
	combo_len = gtk_tree_model_iter_n_children (model, NULL);
	current_index = gtk_combo_box_get_active (combo);
	len = g_list_length (dialog->applet->sounds);
	
	g_debug ("Current index: %d, n sounds: %d", current_index, len);
	
	if (current_index < 0)
		// None selected
		return;
	
	if (current_index >= len) {
		// Select sound file
		g_debug ("Open SOUND file chooser...");
		open_sound_file_chooser (dialog);
		return;
	}
	
	// Valid file selected, update alarm
	item = (AlarmListEntry *) g_list_nth_data (dialog->applet->sounds, current_index);
	g_object_set (dialog->alarm, "sound_file", item->data, NULL);
}

static void
app_combo_changed_cb (GtkComboBox *combo,
					  AlarmSettingsDialog *dialog)
{
	g_debug ("APP Combo_changed");
	
	if (GTK_WIDGET_HAS_FOCUS (dialog->notify_app_command_entry)) {
		g_debug (" ---- Skipping because command_entry has focus!");
		return;
	}
	
	GtkTreeModel *model;
	AlarmListEntry *item;
	guint current_index, len, combo_len;
	
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (dialog->notify_app_combo));
	combo_len = gtk_tree_model_iter_n_children (model, NULL);
	current_index = gtk_combo_box_get_active (combo);
	len = g_list_length (dialog->applet->apps);
	
	if (current_index < 0)
		// None selected
		return;
	
	if (current_index >= len) {
		// Custom command
		g_debug ("CUSTOM command selected...");
		
		g_object_set (dialog->notify_app_command_entry, "sensitive", TRUE, NULL);
		gtk_widget_grab_focus (dialog->notify_app_command_entry);
		return;
	}
	
	g_object_set (dialog->notify_app_command_entry, "sensitive", FALSE, NULL);
	
	
	item = (AlarmListEntry *) g_list_nth_data (dialog->applet->apps, current_index);
	g_object_set (dialog->alarm, "command", item->data, NULL);
}



/*
 * Preview player {{
 */

static void
preview_player_state_cb (MediaPlayer *player, MediaPlayerState state, AlarmSettingsDialog *dialog)
{
	const gchar *stock;
	
	if (state == MEDIA_PLAYER_PLAYING) {
		stock = "gtk-media-stop";
	} else {
		stock = "gtk-media-play";
		
		g_debug ("AlarmSettingsDialog: Freeing media player %p", player);
		
		media_player_free (player);
		dialog->player = NULL;
	}
	
	// Set stock
	gtk_button_set_label (GTK_BUTTON (dialog->notify_sound_preview), stock);
}

void
preview_sound_cb (GtkButton *button,
				  AlarmSettingsDialog *dialog)
{
	if (dialog->player && dialog->player->state == MEDIA_PLAYER_PLAYING) {
		// Stop preview player
		media_player_stop (dialog->player);
	} else {
		// Start preview player
		if (dialog->player == NULL)
			dialog->player = media_player_new (dialog->alarm->sound_file,
											   dialog->alarm->sound_loop,
											   preview_player_state_cb, dialog,
											   media_player_error_cb, dialog->dialog);
	
		g_debug ("AlarmSettingsDialog: preview_start...");
		media_player_start (dialog->player);
	}
}

/*
 * }} Preview player
 */


void
alarm_settings_dialog_close (AlarmSettingsDialog *dialog)
{
	g_hash_table_remove (dialog->applet->edit_alarm_dialogs, dialog->alarm->id);
	
	gtk_widget_destroy (GTK_WIDGET (dialog->dialog));
	
	if (dialog->player) {
		if (dialog->player->state == MEDIA_PLAYER_STOPPED) {
			media_player_free (dialog->player);
		} else {
			media_player_stop (dialog->player);
		}
	}
	
	/* Remove alarm notify handlers! This would otherwise cause segfaults as
	 * the callbacks would look for a non-existant dialog struct.
	 */
	g_signal_handlers_disconnect_matched (dialog->alarm, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, dialog);
	
	// TODO: Why does this cause segfaults once the UI is reused?
	Alarm *a = dialog->alarm;
	g_debug ("FREE AlarmSettingsDialog %p alarm is %p", dialog, a);
	g_free (dialog);
	g_debug ("\talarm is %p", a);
}

static void
alarm_settings_dialog_response_cb (GtkDialog *dialog,
								   gint rid,
								   AlarmSettingsDialog *settings_dialog)
{
	alarm_settings_dialog_close (settings_dialog);
}





/*
 * Dialog creation
 * 
 * TODO: A bug exists which somehow causes some widgets to become NULL
 * 		 and thus crashes the program. Investigate further.
 */

static AlarmSettingsDialog *
alarm_settings_dialog_new (Alarm *alarm, AlarmApplet *applet)
{
	AlarmSettingsDialog *dialog;
	GtkWidget *clock_content, *timer_content;
	AlarmRepeat r;
	gint i;
	
	GladeXML *ui = glade_xml_new (ALARM_UI_XML, "edit-alarm", NULL);
	
	/* Init */
	dialog = g_new0 (AlarmSettingsDialog, 1);
	
	dialog->applet = applet;
	dialog->alarm  = alarm;
	dialog->player = NULL;
	dialog->dialog = glade_xml_get_widget (ui, "edit-alarm");
	
	g_debug ("NEW AlarmSettingsDialog %p for alarm #%d", dialog, alarm->id);
	g_debug ("\talarm is %p");
	
	/* Response from dialog */
	g_signal_connect (dialog->dialog, "response", 
					  G_CALLBACK (alarm_settings_dialog_response_cb), dialog);	
	
	/*
	 * TYPE TOGGLE BUTTONS
	 */
	dialog->clock_toggle = glade_xml_get_widget (ui, "toggle-clock");
	clock_content = create_img_label ("Alarm Clock", "alarm-clock");
	
	dialog->timer_toggle = glade_xml_get_widget (ui, "toggle-timer");
	timer_content = create_img_label ("Timer", "alarm-timer");
	
	gtk_container_add (GTK_CONTAINER (dialog->clock_toggle), clock_content);
	gtk_widget_show_all (GTK_WIDGET (dialog->clock_toggle));
	
	gtk_container_add (GTK_CONTAINER (dialog->timer_toggle), timer_content);
	gtk_widget_show_all (GTK_WIDGET (dialog->timer_toggle));
	
	/*
	 * GENERAL SETTINGS
	 */
	dialog->label_entry = glade_xml_get_widget (ui, "label-entry");
	gtk_widget_grab_focus (dialog->label_entry);
	
	dialog->hour_spin = glade_xml_get_widget (ui, "hour-spin");
	dialog->min_spin = glade_xml_get_widget (ui, "minute-spin");
	dialog->sec_spin = glade_xml_get_widget (ui, "second-spin");
	
	/*
	 * REPEAT SETTINGS
	 */
	dialog->repeat_expand = glade_xml_get_widget (ui, "repeat-expand");
	dialog->repeat_label  = glade_xml_get_widget (ui, "repeat-label");
	
	/* The check buttons have the same name as the 3 letter
	 * string representation of the day.
	 */
	for (r = ALARM_REPEAT_SUN, i = 0; r <= ALARM_REPEAT_SAT; r = 1 << ++i) {
		dialog->repeat_check[i] = glade_xml_get_widget (ui, alarm_repeat_to_string (r));
	}
	
	/*
	 * NOTIFY SETTINGS
	 */
	dialog->notify_sound_radio       = glade_xml_get_widget (ui, "sound-radio");
	dialog->notify_sound_box         = glade_xml_get_widget (ui, "sound-box");
	dialog->notify_sound_combo       = glade_xml_get_widget (ui, "sound-combo");
	dialog->notify_sound_preview     = glade_xml_get_widget (ui, "sound-play");
	dialog->notify_sound_loop_check  = glade_xml_get_widget (ui, "sound-loop-check");
	dialog->notify_app_radio         = glade_xml_get_widget (ui, "app-radio");
	dialog->notify_app_box           = glade_xml_get_widget (ui, "app-box");
	dialog->notify_app_combo         = glade_xml_get_widget (ui, "app-combo");
	dialog->notify_app_command_box   = glade_xml_get_widget (ui, "app-command-box");
	dialog->notify_app_command_entry = glade_xml_get_widget (ui, "app-command-entry");
	dialog->notify_bubble_check      = glade_xml_get_widget (ui, "notify-bubble-check");
	
	
	/*
	 * Load apps list
	 */
	alarm_applet_apps_load (applet);
	
	/*
	 * Populate widgets
	 */
	alarm_settings_update (dialog);
	
	/* Got libnotify? */
#ifndef HAVE_LIBNOTIFY
	g_object_set (dialog->notify_bubble_check, "sensitive", FALSE, NULL);
	gtk_widget_set_tooltip_text (GTK_WIDGET (dialog->notify_bubble_check), _("This feature requires libnotify to be installed"));
#endif
	
	/*
	 * Bind widgets
	 */
	alarm_bind (alarm, "message", dialog->label_entry, "text");
	alarm_bind (alarm, "sound-repeat", dialog->notify_sound_loop_check, "active");
	alarm_bind (alarm, "command", dialog->notify_app_command_entry, "text");
	alarm_bind (alarm, "notify-bubble", dialog->notify_bubble_check, "active");
	
	/*
	 * Special widgets require special attention!
	 */
	
	/* type */
	g_signal_connect (alarm, "notify::type", G_CALLBACK (alarm_type_changed), dialog);
	g_signal_connect (dialog->clock_toggle, "toggled", G_CALLBACK (type_toggle_cb), dialog);
	g_signal_connect (dialog->timer_toggle, "toggled", G_CALLBACK (type_toggle_cb), dialog);
	
	/* time/timer */
	g_signal_connect (alarm, "notify::time", G_CALLBACK (alarm_time_changed), dialog);
	g_signal_connect (alarm, "notify::timer", G_CALLBACK (alarm_timer_changed), dialog);
	
	g_signal_connect (dialog->hour_spin, "value-changed", G_CALLBACK (time_changed_cb), dialog);
	g_signal_connect (dialog->min_spin, "value-changed", G_CALLBACK (time_changed_cb), dialog);
	g_signal_connect (dialog->sec_spin, "value-changed", G_CALLBACK (time_changed_cb), dialog);
	
	/* repeat */
	g_signal_connect (alarm, "notify::repeat", G_CALLBACK (alarm_repeat_changed), dialog);
	for (i = 0; i < 7; i++)
		g_signal_connect (dialog->repeat_check[i], "toggled", G_CALLBACK (repeat_changed_cb), dialog);
	
	/* notify type */
	g_signal_connect (alarm, "notify::notify-type", G_CALLBACK (alarm_notify_type_changed), dialog);
	g_signal_connect (dialog->notify_sound_radio, "toggled", G_CALLBACK (notify_type_changed_cb), dialog);
	g_signal_connect (dialog->notify_app_radio, "toggled", G_CALLBACK (notify_type_changed_cb), dialog);
	
	/* sound file */
	g_signal_connect (alarm, "notify::sound-file", 
					  G_CALLBACK (alarm_settings_sound_file_changed), dialog);
	g_signal_connect (dialog->notify_sound_combo, "changed",
					  G_CALLBACK (sound_combo_changed_cb), dialog);
	
	g_signal_connect (dialog->notify_sound_preview, "clicked",
					  G_CALLBACK (preview_sound_cb), dialog);
	
	/* app / command */
	g_signal_connect (alarm, "notify::command",
					  G_CALLBACK (alarm_settings_command_changed), dialog);
	
	g_signal_connect (dialog->notify_app_combo, "changed",
					  G_CALLBACK (app_combo_changed_cb), dialog);
	
	/*g_signal_connect (dialog->notify_app_command_entry, "changed",
					  G_CALLBACK (app_command_changed_cb), dialog);*/
	
	g_object_unref (ui);
	
	return dialog;
}

void
display_edit_alarm_dialog (AlarmApplet *applet, Alarm *alarm)
{
	AlarmSettingsDialog *dialog;
	
	// Check if a dialog is already open for this alarm
	dialog = (AlarmSettingsDialog *)g_hash_table_lookup (applet->edit_alarm_dialogs, alarm->id);
	
	if (dialog) {
		// Already open
		gtk_window_present (GTK_WINDOW (dialog->dialog));
		return;
	}
	
	dialog = alarm_settings_dialog_new (alarm, applet);
	g_hash_table_insert (applet->edit_alarm_dialogs, alarm->id, dialog);
}
