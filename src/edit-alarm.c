#include "edit-alarm.h"
#include "alarm-applet.h"
#include "alarm.h"

static void
alarm_settings_update_time (AlarmSettingsDialog *dialog);

static void
time_changed_cb (GtkSpinButton *spinbutton, gpointer data);


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
	} else {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->timer_toggle), TRUE);
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

void
alarm_settings_update_notify_type (AlarmSettingsDialog *dialog)
{
	// Enable selected
	switch (dialog->alarm->notify_type) {
	case NOTIFY_COMMAND:
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
	GtkTreeModel *model = NULL;
	GtkTreeIter iter;
	GtkTreePath *path;
	GtkIconTheme *theme;
	GdkPixbuf *pixbuf;
	GList *l;
	guint pos, len, combo_len;
	gint sound_pos = -1;
	gchar *tmp;
	
	/*if (applet->notify_sound_file == NULL) {
		g_debug ("pref_update_sound_file IS NULL or EMPTY");
		// Select first in list
		gtk_combo_box_set_active (GTK_COMBO_BOX (applet->pref_notify_sound_combo), 0);
		return;
	}*/
	
	/*model = gtk_combo_box_get_model (GTK_COMBO_BOX (dialog->notify_sound_combo));
	combo_len = gtk_tree_model_iter_n_children (model, NULL);
	len = g_list_length (applet->sounds);
	
	pos = len;
	
	// Check if our list in model is up-to-date
	// We will only detect changes in size
	// Separator + Select sound file ... takes 2 spots
	/*if (len > combo_len - 2) {
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
	}*/
	
			
	
	// Look for the selected sound file
	for (l = dialog->applet->sounds, pos = 0; l != NULL; l = l->next, pos++) {
		tmp = (gchar *)l->data;
		if (strcmp (tmp, dialog->alarm->sound_file) == 0) {
			// Match!
			sound_pos = pos;
			gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->notify_sound_combo), pos);
			break;
		}
	}
	
	/*if (sound_pos > 0) {
		
	}*/
	
	/*g_debug ("set_row_sep_func to %d", pos);
	gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (dialog->notify_sound_combo),
										  is_separator, GINT_TO_POINTER (pos), NULL);*/
}

static void
alarm_settings_update (AlarmSettingsDialog *dialog)
{
	Alarm *alarm = ALARM (dialog->alarm);
	
	g_object_set (dialog->label_entry, "text", alarm->message, NULL);
	
	alarm_settings_update_type (dialog);
	alarm_settings_update_time (dialog);
	alarm_settings_update_notify_type (dialog);
	alarm_settings_update_sound (dialog);
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

void
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

void
alarm_notify_type_changed (GObject *object, 
						   GParamSpec *param,
						   gpointer data)
{
	AlarmSettingsDialog *dialog = (AlarmSettingsDialog *)data;
	
	alarm_settings_update_notify_type (dialog);
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
alarm_settings_dialog_response_cb (GtkDialog *dialog,
								   gint rid,
								   AlarmSettingsDialog *settings_dialog)
{
	gtk_widget_destroy (GTK_WIDGET (dialog));
	
	// TODO: Why does this cause a segfault?
	//g_free (settings_dialog);
}





/*
 * Dialog creation
 */

static AlarmSettingsDialog *
alarm_settings_dialog_new (Alarm *alarm, AlarmApplet *applet)
{
	AlarmSettingsDialog *dialog;
	GtkWidget *clock_content, *timer_content;
	
	GladeXML *ui = glade_xml_new (ALARM_UI_XML, "edit-alarm", NULL);
	
	/* Init */
	dialog = g_new (AlarmSettingsDialog, 1);
	
	dialog->applet = applet;
	dialog->alarm  = alarm;
	dialog->dialog = glade_xml_get_widget (ui, "edit-alarm");
	
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
	
	/* Load sounds list */
	load_stock_sounds_list (applet);
	
	/* Fill stock sounds */
	fill_combo_box (GTK_COMBO_BOX (dialog->notify_sound_combo),
					applet->sounds, _("Select sound file..."));
	
	/*
	 * Populate widgets
	 */
	alarm_settings_update (dialog);
	
	
	
	/*
	 * glade_xml_get_widget() caches its widgets. 
	 * So if g_object_destroy() got called on it, the next call to 
	 * glade_xml_get_widget() would return a pointer to the destroyed widget.
	 * Therefore we must call g_object_ref() on the widgets we intend to reuse.
	 *
	 * TODO: Is this correct? Was not necessary with other windows?
	 * Without these the dialog bugs completely the second time it's opened.
	 */
	g_object_ref (dialog->dialog);
	g_object_ref (dialog->clock_toggle);
	g_object_ref (dialog->timer_toggle);
	g_object_ref (dialog->label_entry);
	g_object_ref (dialog->hour_spin);
	g_object_ref (dialog->min_spin);
	g_object_ref (dialog->sec_spin);
	
	g_object_ref (dialog->notify_sound_radio);
	g_object_ref (dialog->notify_sound_box);
	g_object_ref (dialog->notify_sound_combo);
	g_object_ref (dialog->notify_sound_preview);
	g_object_ref (dialog->notify_sound_loop_check);
	g_object_ref (dialog->notify_app_radio);
	g_object_ref (dialog->notify_app_box);
	g_object_ref (dialog->notify_app_combo);
	g_object_ref (dialog->notify_app_command_box);
	g_object_ref (dialog->notify_app_command_entry);
	g_object_ref (dialog->notify_bubble_check);
	
	
	/*
	 * Bind widgets
	 */
	alarm_bind (alarm, "message", dialog->label_entry, "text");
	
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
	
	/* notify type */
	g_signal_connect (alarm, "notify::notify-type", G_CALLBACK (alarm_notify_type_changed), dialog);
	g_signal_connect (dialog->notify_sound_radio, "toggled", G_CALLBACK (notify_type_changed_cb), dialog);
	g_signal_connect (dialog->notify_app_radio, "toggled", G_CALLBACK (notify_type_changed_cb), dialog);
	
	return dialog;
}

void
display_add_alarm_dialog (AlarmApplet *applet)
{
	Alarm *alarm;
	AlarmSettingsDialog *dialog;
	
	/*
	 * Create new alarm, will fall back to defaults.
	 */
	alarm = alarm_new (applet->gconf_dir, -1);
	
	dialog = alarm_settings_dialog_new (alarm, applet);
}

void
display_edit_alarm_dialog (AlarmApplet *applet, Alarm *alarm)
{
	AlarmSettingsDialog *dialog;
	
	dialog = alarm_settings_dialog_new (alarm, applet);
}
