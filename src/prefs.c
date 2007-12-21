#include "prefs.h"

void
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
	
	if (applet->preview_player) {
		media_player_stop (applet->preview_player);
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	applet->preferences_dialog = NULL;
}

void
pref_update_label_show (AlarmApplet *applet)
{
	g_object_set (applet->pref_label_show, "active", applet->show_label, NULL);
	g_object_set (applet->pref_label_type_box, "sensitive", applet->show_label, NULL);
}

void
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

void
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

void
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

void
pref_update_sound_loop (AlarmApplet *applet)
{
	g_object_set (applet->pref_notify_sound_loop, "active", applet->notify_sound_loop, NULL);
}

void
pref_update_command (AlarmApplet *applet)
{
	g_object_set (applet->pref_notify_app_command_entry, "text", applet->notify_command, NULL);
	
	
}

void
pref_update_show_bubble (AlarmApplet *applet)
{
	g_object_set (applet->pref_notify_bubble, "active", applet->notify_bubble, NULL);
}

void
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
					  G_CALLBACK (pref_notify_app_combo_changed_cb), applet);
	
	g_signal_connect (applet->pref_notify_app_command_entry, "changed",
					  G_CALLBACK (pref_notify_app_command_changed_cb), applet);
	
	// Bubble
	g_signal_connect (applet->pref_notify_bubble, "toggled",
					  G_CALLBACK (pref_notify_bubble_changed_cb), applet);
	
	// Show it all
	gtk_widget_show_all (GTK_WIDGET (applet->preferences_dialog));
}

void
pref_label_show_changed_cb (GtkToggleButton *togglebutton,
							AlarmApplet *applet)
{
	gboolean value = gtk_toggle_button_get_active (togglebutton);
	
	panel_applet_gconf_set_bool (applet->parent, KEY_SHOW_LABEL, value, NULL);
}

void
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

void
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

void
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

void
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

void
pref_notify_sound_loop_changed_cb (GtkToggleButton *togglebutton,
							AlarmApplet *applet)
{
	gboolean value = gtk_toggle_button_get_active (togglebutton);
	
	panel_applet_gconf_set_bool (applet->parent, KEY_SOUND_LOOP, value, NULL);
}

void
pref_play_sound_cb (GtkButton *button,
					AlarmApplet *applet)
{
	if (applet->preview_player && applet->preview_player->state == MEDIA_PLAYER_PLAYING) {
		// Stop preview player
		media_player_stop (applet->preview_player);
	} else {
		// Start preview player
		player_preview_start (applet);
	}
}

void
pref_notify_app_combo_changed_cb (GtkComboBox *combo,
								  AlarmApplet *applet)
{
	g_debug ("APP Combo_changed");
	
	GtkTreeModel *model;
	AlarmListEntry *item;
	guint current_index, len, combo_len;
	
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (applet->pref_notify_app_combo));
	combo_len = gtk_tree_model_iter_n_children (model, NULL);
	current_index = gtk_combo_box_get_active (combo);
	len = g_list_length (applet->apps);
	
	if (current_index < 0)
		// None selected
		return;
	
	if (current_index >= len) {
		// Select sound file
		g_debug ("CUSTOM command selected...");
		
		g_object_set (applet->pref_notify_app_command_entry, "sensitive", TRUE, NULL);
		return;
	}
	
	g_object_set (applet->pref_notify_app_command_entry, "sensitive", FALSE, NULL);
	
	item = (AlarmListEntry *) g_list_nth_data (applet->apps, current_index);
	
	panel_applet_gconf_set_string (applet->parent, KEY_COMMAND, item->data, NULL);
}

void
pref_notify_app_command_changed_cb (GtkEditable *editable,
								AlarmApplet *applet)
{
	const gchar *value = gtk_entry_get_text (GTK_ENTRY (editable));
	
	panel_applet_gconf_set_string (applet->parent, KEY_COMMAND, value, NULL);
}

void
pref_notify_bubble_changed_cb (GtkToggleButton *togglebutton,
							AlarmApplet *applet)
{
	gboolean value = gtk_toggle_button_get_active (togglebutton);
	
	panel_applet_gconf_set_bool (applet->parent, KEY_NOTIFY_BUBBLE, value, NULL);
}