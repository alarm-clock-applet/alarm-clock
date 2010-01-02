/*
 * alarm-settings.c -- Alarm settings dialog
 * 
 * Copyright (C) 2007-2008 Johannes H. Jensen <joh@pseudoberries.com>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 * Authors:
 * 		Johannes H. Jensen <joh@pseudoberries.com>
 */

#include "alarm-settings.h"
#include "alarm-applet.h"
#include "alarm.h"
#include "player.h"

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#define BLOCK(instance, func)   \
    g_signal_handlers_block_matched ((instance),            \
                                     G_SIGNAL_MATCH_FUNC,   \
                                     0, 0, NULL, (func), NULL)

#define UNBLOCK(instance, func)   \
    g_signal_handlers_unblock_matched ((instance),            \
                                       G_SIGNAL_MATCH_FUNC,   \
                                       0, 0, NULL, (func), NULL)



/*
 * GUI callbacks
 */

void
alarm_settings_changed_type (GtkToggleButton *toggle, gpointer data);

void
alarm_settings_changed_label (GtkEditable *editable, gpointer data);

void
alarm_settings_changed_time (GtkSpinButton *spinbutton, gpointer data);

void
alarm_settings_changed_repeat (GtkToggleButton *togglebutton, gpointer data);

void
alarm_settings_changed_snooze (GtkSpinButton *spinbutton, gpointer data);

void
alarm_settings_changed_snooze_check (GtkToggleButton *togglebutton, gpointer data);

void
alarm_settings_changed_notify_type (GtkToggleButton *togglebutton, gpointer data);

void
alarm_settings_changed_sound (GtkComboBox *combo, gpointer data);

void
alarm_settings_changed_sound_repeat (GtkToggleButton *togglebutton, gpointer data);

void
alarm_settings_changed_app (GtkComboBox *combo, gpointer data);

void
alarm_settings_changed_command (GtkEditable *editable, gpointer data);



static void
alarm_settings_update_time (AlarmSettingsDialog *dialog);

static void
time_changed_cb (GtkSpinButton *spinbutton, gpointer data);

void
alarm_settings_changed_sound (GtkComboBox *combo, gpointer data);

void
alarm_settings_changed_app (GtkComboBox *combo, gpointer data);



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
    g_debug ("AlarmSettingsDialog: update_type()");
    
    BLOCK (dialog->clock_toggle, alarm_settings_changed_type);
    BLOCK (dialog->timer_toggle, alarm_settings_changed_type);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->clock_toggle), 
        dialog->alarm->type == ALARM_TYPE_CLOCK);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->timer_toggle), 
        dialog->alarm->type == ALARM_TYPE_TIMER);
    
    if (dialog->alarm->type == ALARM_TYPE_CLOCK) {
		gtk_widget_set_sensitive(dialog->repeat_expand, TRUE);
	} else {
		gtk_widget_set_sensitive(dialog->repeat_expand, FALSE);
	}

    UNBLOCK (dialog->clock_toggle, alarm_settings_changed_type);
    UNBLOCK (dialog->timer_toggle, alarm_settings_changed_type);
}

static void
alarm_settings_update_label (AlarmSettingsDialog *dialog)
{
    g_debug ("AlarmSettingsDialog: update_label()");
    
    BLOCK (dialog->label_entry, alarm_settings_changed_label);
    
    g_object_set (dialog->label_entry, "text", dialog->alarm->message, NULL);
    
    UNBLOCK (dialog->label_entry, alarm_settings_changed_label);
}

static void
alarm_settings_update_time (AlarmSettingsDialog *dialog)
{
	struct tm *tm;

    g_debug ("AlarmSettingsDialog: update_time()");
	
	tm = alarm_get_time(dialog->alarm);

    BLOCK (dialog->hour_spin, alarm_settings_changed_time);
    BLOCK (dialog->min_spin, alarm_settings_changed_time);
    BLOCK (dialog->sec_spin, alarm_settings_changed_time);
    
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->hour_spin), tm->tm_hour);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->min_spin), tm->tm_min);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->sec_spin), tm->tm_sec);

    UNBLOCK (dialog->hour_spin, alarm_settings_changed_time);
    UNBLOCK (dialog->min_spin, alarm_settings_changed_time);
    UNBLOCK (dialog->sec_spin, alarm_settings_changed_time);
}

static void
alarm_settings_update_repeat (AlarmSettingsDialog *dialog)
{
	AlarmRepeat r;
	gint i;
	gboolean check;
	gchar *label;

    g_debug ("AlarmSettingsDialog: update_repeat()");
	
	/*
	 * Update check boxes
	 */
	for (r = ALARM_REPEAT_SUN, i = 0; r <= ALARM_REPEAT_SAT; r = 1 << ++i) {
		check = (dialog->alarm->repeat & r) != 0;
		
		// Activate the appropriate widget
        BLOCK (dialog->repeat_check[i], alarm_settings_changed_repeat);
        
		g_object_set (dialog->repeat_check[i], "active", check, NULL);
        
        UNBLOCK (dialog->repeat_check[i], alarm_settings_changed_repeat);
	}
	

    /*
     * Update fancy expander label
     */
    label = alarm_repeat_to_pretty (dialog->alarm->repeat);
    g_object_set (dialog->repeat_label, "label", label, NULL);
    g_free (label);
}

static void
alarm_settings_update_snooze (AlarmSettingsDialog *dialog)
{
    g_debug ("AlarmSettingsDialog: update_snooze()");
    
    BLOCK (dialog->snooze_check, alarm_settings_changed_snooze_check);
    BLOCK (dialog->snooze_spin, alarm_settings_changed_snooze);
    
	g_object_set (dialog->snooze_spin, "value", (gdouble)dialog->alarm->snooze, NULL);	
	g_object_set (dialog->snooze_check, "active", dialog->alarm->snooze > 0, NULL);

    UNBLOCK (dialog->snooze_check, alarm_settings_changed_snooze_check);
    UNBLOCK (dialog->snooze_spin, alarm_settings_changed_snooze);
}

static void
alarm_settings_update_notify_type (AlarmSettingsDialog *dialog)
{
    g_debug ("AlarmSettingsDialog: update_notify_type()");
    
	// Enable selected
	switch (dialog->alarm->notify_type) {
        case ALARM_NOTIFY_COMMAND:

            BLOCK (dialog->notify_app_radio, alarm_settings_changed_notify_type);

            g_object_set (dialog->notify_app_radio, "active", TRUE, NULL);
            g_object_set (dialog->notify_app_box, "sensitive", TRUE, NULL);

            // Disable others
            g_object_set (dialog->notify_sound_box, "sensitive", FALSE, NULL);

            if (dialog->player && dialog->player->state == MEDIA_PLAYER_PLAYING) {
                // Stop preview player
                media_player_stop (dialog->player);
            }

            UNBLOCK (dialog->notify_app_radio, alarm_settings_changed_notify_type);

            break;
        default:
            // NOTIFY_SOUND
            BLOCK (dialog->notify_sound_radio, alarm_settings_changed_notify_type);
            
            g_object_set (dialog->notify_sound_radio, "active", TRUE, NULL);
            g_object_set (dialog->notify_sound_box, "sensitive", TRUE, NULL);

            // Disable others
            g_object_set (dialog->notify_app_box, "sensitive", FALSE, NULL);

            UNBLOCK (dialog->notify_sound_radio, alarm_settings_changed_notify_type);
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

    g_debug ("AlarmSettingsDialog: update_sound()");

    BLOCK (dialog->notify_sound_combo, alarm_settings_changed_sound);
	
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

    UNBLOCK (dialog->notify_sound_combo, alarm_settings_changed_sound);
}

static void
alarm_settings_update_sound_repeat (AlarmSettingsDialog *dialog)
{
    g_debug ("AlarmSettingsDialog: update_sound_repeat()");
    
    BLOCK (dialog->notify_sound_loop_check, alarm_settings_changed_sound_repeat);
    
	g_object_set (dialog->notify_sound_loop_check, 
        "active", dialog->alarm->sound_loop, NULL);
    
    UNBLOCK (dialog->notify_sound_loop_check, alarm_settings_changed_sound_repeat);
}

static void
alarm_settings_update_app (AlarmSettingsDialog *dialog)
{
	AlarmListEntry *item;
	GList *l;
	guint pos, len, combo_len;
	gboolean custom = FALSE;

    g_debug ("AlarmSettingsDialog: update_app()");
	
//	g_debug ("alarm_settings_update_app (%p): app_combo: %p, applet: %p, apps: %p", dialog, dialog->notify_app_combo, dialog->applet, dialog->applet->apps);
//	g_debug ("alarm_settings_update_app setting entry to %s", dialog->alarm->command);

    BLOCK (dialog->notify_app_combo, alarm_settings_changed_app);
    
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

    UNBLOCK (dialog->notify_app_combo, alarm_settings_changed_app);
}

static void
alarm_settings_update_app_command (AlarmSettingsDialog *dialog)
{
    g_debug ("AlarmSettingsDialog: update_app_command()");
    
    BLOCK (dialog->notify_app_command_entry, alarm_settings_changed_command);
    
    g_object_set (dialog->notify_app_command_entry, "text", dialog->alarm->command, NULL);
    
    UNBLOCK (dialog->notify_app_command_entry, alarm_settings_changed_command);
}

static void
alarm_settings_update (AlarmSettingsDialog *dialog)
{
	Alarm *alarm = ALARM (dialog->alarm);
	
	alarm_settings_update_type (dialog);
    alarm_settings_update_label (dialog);
	alarm_settings_update_time (dialog);
	alarm_settings_update_repeat (dialog);
	alarm_settings_update_snooze (dialog);
	alarm_settings_update_notify_type (dialog);
	alarm_settings_update_sound (dialog);
    alarm_settings_update_sound_repeat (dialog);
	alarm_settings_update_app (dialog);
    alarm_settings_update_app_command (dialog);
}






/*
 * Alarm object callbacks
 */

static void
alarm_changed (GObject *object, 
               GParamSpec *param,
               gpointer data)
{
    AlarmSettingsDialog *dialog = (AlarmSettingsDialog *)data;

    const gchar *name = param->name;

    g_debug("AlarmSettingsDialog: alarm_changed: %s", name);

//    alarm_settings_update (dialog);
//    return;

    if (g_strcmp0 (name, "type") == 0) {
        alarm_settings_update_type (dialog);
    }
    
    if (g_strcmp0 (name, "message") == 0) {
        alarm_settings_update_label (dialog);
    }
    
    if (g_strcmp0 (name, "time") == 0) {
        alarm_settings_update_time (dialog);
    }

    if (g_strcmp0 (name, "repeat") == 0) {
        alarm_settings_update_repeat (dialog);
    }

    if (g_strcmp0 (name, "snooze") == 0) {
        alarm_settings_update_snooze (dialog);
    }

    if (g_strcmp0 (name, "notify-type") == 0) {
        alarm_settings_update_notify_type (dialog);
    }

    if (g_strcmp0 (name, "sound-file") == 0) {
        // Block sound combo signals to prevent infinite loop
	    // because the "changed" signal will be emitted when we
	    // change the combo box tree model.
	    /*g_signal_handlers_block_matched (dialog->notify_sound_combo, 
									     G_SIGNAL_MATCH_FUNC, 
									     0, 0, NULL, alarm_settings_changed_sound, NULL);
	    */
	    // Update UI
	    alarm_settings_update_sound (dialog);
	
	    // Unblock combo signals
	    /*g_signal_handlers_unblock_matched (dialog->notify_sound_combo, 
									       G_SIGNAL_MATCH_FUNC,
									       0, 0, NULL, alarm_settings_changed_sound, NULL);
         */
    }

    if (g_strcmp0 (name, "sound-repeat") == 0) {
       	g_object_set (dialog->notify_sound_loop_check, "active", dialog->alarm->sound_loop, NULL);
    }

    if (g_strcmp0 (name, "command") == 0) {
        // Block sound combo signals to prevent infinite loop
	    // because the "changed" signal will be emitted when we
	    // change the combo box tree model.
	    /*g_signal_handlers_block_matched (dialog->notify_app_combo, 
									     G_SIGNAL_MATCH_FUNC, 
									     0, 0, NULL, alarm_settings_changed_app, NULL);
	*/
	    // Update UI
	    alarm_settings_update_app (dialog);
	
	    // Unblock combo signals
	    /*g_signal_handlers_unblock_matched (dialog->notify_app_combo, 
									       G_SIGNAL_MATCH_FUNC,
									       0, 0, NULL, alarm_settings_changed_app, NULL);*/
    }
/*
    if (g_strcmp0 (name, "") == 0) {

    }

    if (g_strcmp0 (name, "") == 0) {

    }
    */
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

void
alarm_settings_changed_type (GtkToggleButton *toggle, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
	AlarmSettingsDialog *dialog = applet->settings_dialog;
    
	GtkWidget *toggle2 = (GTK_WIDGET (toggle) == dialog->clock_toggle) ? dialog->timer_toggle : dialog->clock_toggle;
	gboolean toggled = gtk_toggle_button_get_active(toggle);

    g_assert (dialog->alarm != NULL);
    g_debug ("alarm_settings_changed_type");
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle2), !toggled);
	
	if (GTK_WIDGET (toggle) == dialog->clock_toggle && toggled) {
		g_object_set (dialog->alarm, "type", ALARM_TYPE_CLOCK, NULL);
        gtk_widget_set_sensitive(dialog->repeat_expand, TRUE);
	} else {
		g_object_set (dialog->alarm, "type", ALARM_TYPE_TIMER, NULL);
        gtk_widget_set_sensitive(dialog->repeat_expand, FALSE);
	}
    
    // TODO: Why?
//	alarm_settings_changed_time (GTK_SPIN_BUTTON (dialog->hour_spin), dialog);
}

void
alarm_settings_changed_label (GtkEditable *editable,
                              gpointer     data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
	AlarmSettingsDialog *dialog = applet->settings_dialog;
    const gchar *text;

    g_assert (dialog->alarm != NULL);
    
    text = gtk_entry_get_text (GTK_ENTRY (editable));

    g_debug ("label_changed: %s", text);

    g_object_set (dialog->alarm, "message", text, NULL);
}

void
alarm_settings_changed_time (GtkSpinButton *spinbutton, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
	AlarmSettingsDialog *dialog = applet->settings_dialog;
	guint hour, min, sec;

    g_assert (dialog->alarm != NULL);
    
	hour = gtk_spin_button_get_value (GTK_SPIN_BUTTON (dialog->hour_spin));
	min = gtk_spin_button_get_value (GTK_SPIN_BUTTON (dialog->min_spin));
	sec = gtk_spin_button_get_value (GTK_SPIN_BUTTON (dialog->sec_spin));
	
	alarm_set_time (dialog->alarm, hour, min, sec);
}

void
alarm_settings_changed_repeat (GtkToggleButton *togglebutton, gpointer data)
{
	AlarmApplet *applet = (AlarmApplet *)data;
	AlarmSettingsDialog *dialog = applet->settings_dialog;

    g_assert (dialog->alarm != NULL);
	
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

void
alarm_settings_changed_snooze (GtkSpinButton *spinbutton, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
	AlarmSettingsDialog *dialog = applet->settings_dialog;

    g_assert (dialog->alarm != NULL);

    gint val = gtk_spin_button_get_value (GTK_SPIN_BUTTON (dialog->snooze_spin));
	
	g_object_set (dialog->alarm, "snooze", val, NULL);
    g_object_set (dialog->snooze_check, "active", dialog->alarm->snooze > 0, NULL);
}

void
alarm_settings_changed_snooze_check (GtkToggleButton *togglebutton, gpointer data)
{
	AlarmApplet *applet = (AlarmApplet *)data;
	AlarmSettingsDialog *dialog = applet->settings_dialog;

    g_assert (dialog->alarm != NULL);
	g_debug("alarm_settings_changed_snooze_check");
	if (gtk_toggle_button_get_active (togglebutton) && dialog->alarm->snooze == 0) {
		g_object_set (dialog->alarm, "snooze", ALARM_DEF_SNOOZE, NULL);
	} else if (!gtk_toggle_button_get_active (togglebutton) && dialog->alarm->snooze > 0) {
		g_object_set (dialog->alarm, "snooze", 0, NULL);
	}
}

void
alarm_settings_changed_notify_type (GtkToggleButton *togglebutton, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
	AlarmSettingsDialog *dialog = applet->settings_dialog;
    
	const gchar *name = gtk_widget_get_name (GTK_WIDGET (togglebutton));
	gboolean value    = gtk_toggle_button_get_active (togglebutton);
	
	if (!value) {
		// Not checked, not interested
		return;
	}

    g_assert (dialog->alarm != NULL);
	
	g_debug ("notify_type_changed: %s", name);
	
	if (strcmp (name, "app-radio") == 0) {
		g_object_set (dialog->alarm, "notify_type", ALARM_NOTIFY_COMMAND, NULL);
	} else {
		g_object_set (dialog->alarm, "notify_type", ALARM_NOTIFY_SOUND, NULL);
	}
	
	alarm_settings_update_notify_type (dialog);
}

void
alarm_settings_changed_sound (GtkComboBox *combo, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
	AlarmSettingsDialog *dialog = applet->settings_dialog;
    
	g_debug ("SOUND Combo_changed");

    g_assert (dialog->alarm != NULL);
	
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

void
alarm_settings_changed_sound_repeat (GtkToggleButton *togglebutton, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
	AlarmSettingsDialog *dialog = applet->settings_dialog;

    g_assert (dialog->alarm != NULL);

    g_object_set (dialog->alarm, "sound-repeat", gtk_toggle_button_get_active (togglebutton), NULL);
}

void
alarm_settings_changed_app (GtkComboBox *combo, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
	AlarmSettingsDialog *dialog = applet->settings_dialog;
    
	g_debug ("APP Combo_changed");

    g_assert (dialog->alarm != NULL);
	
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

void
alarm_settings_changed_command (GtkEditable *editable, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
	AlarmSettingsDialog *dialog = applet->settings_dialog;

    g_assert (dialog->alarm != NULL);

    g_object_set (dialog->alarm, "command", gtk_entry_get_text (GTK_ENTRY (editable)), NULL);
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
alarm_settings_sound_preview (GtkButton *button, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
	AlarmSettingsDialog *dialog = applet->settings_dialog;
    
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


/*
 * Clear settings dialog for reuse
 *
 * Stops any running media players
 * Disassociates the dialog from any alarm
 */
static void
alarm_settings_dialog_clear (AlarmSettingsDialog *dialog)
{
    if (dialog->player) {
		if (dialog->player->state == MEDIA_PLAYER_STOPPED) {
			media_player_free (dialog->player);
		} else {
			media_player_stop (dialog->player);
		}
	}

    if (dialog->alarm) {
	    /* Remove alarm notify handlers! */
        int matched = g_signal_handlers_disconnect_matched (dialog->alarm, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, alarm_changed, NULL);
//	    int matched = g_signal_handlers_disconnect_matched (dialog->alarm, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, dialog);

        g_debug("settings CLEAR alarm %p: %d handlers removed", dialog->alarm, matched);

        dialog->alarm = NULL;

        /* Remove signal handlers */
        //g_signal_handlers_disconnect_by_func
    }
}

void
alarm_settings_dialog_close (AlarmSettingsDialog *dialog)
{
//	g_hash_table_remove (dialog->applet->edit_alarm_dialogs, dialog->alarm->id);
	
//	gtk_widget_destroy (GTK_WIDGET (dialog->dialog))
	
    alarm_settings_dialog_clear (dialog);

    gtk_widget_hide (dialog->dialog);
}

void
alarm_settings_dialog_response (GtkDialog *dialog,
								gint rid,
								gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
	AlarmSettingsDialog *settings_dialog = applet->settings_dialog;
    
    g_debug ("alarm_settings_dialog_response %d", rid);

    alarm_settings_dialog_close (settings_dialog);
}

/*
 * Associate a settings dialog with an alarm
 *
 * Clears any previously associated alarms
 */
static void
alarm_settings_dialog_set_alarm (AlarmSettingsDialog *dialog, Alarm *alarm)
{
    int i;
    
    // Clear dialog
    alarm_settings_dialog_clear (dialog);

    // Set alarm
    dialog->alarm = alarm;
    
    // Populate widgets
	alarm_settings_update (dialog);

    // Notify of change to alarm
    g_signal_connect (alarm, "notify", G_CALLBACK (alarm_changed), dialog);
}


/*
 * Create a new settings dialog
 */
AlarmSettingsDialog *
alarm_settings_dialog_new (AlarmApplet *applet)
{
	AlarmSettingsDialog *dialog;
	GtkWidget *clock_content, *timer_content, *snooze_label;
	AlarmRepeat r;
	gint i;

    GtkBuilder *builder = applet->ui;
	
	// Initialize struct
	dialog = g_new0 (AlarmSettingsDialog, 1);
	
	dialog->applet = applet;
	dialog->player = NULL;
	dialog->dialog = GTK_WIDGET (gtk_builder_get_object (builder, "alarm-settings-dialog"));
	
	// TYPE TOGGLE BUTTONS
	dialog->clock_toggle = GTK_WIDGET (gtk_builder_get_object (builder, "toggle-clock"));
	dialog->timer_toggle = GTK_WIDGET (gtk_builder_get_object (builder, "toggle-timer"));
	
	// GENERAL SETTINGS
	dialog->label_entry = GTK_WIDGET (gtk_builder_get_object (builder, "label-entry"));
	gtk_widget_grab_focus (dialog->label_entry);
	
	dialog->hour_spin = GTK_WIDGET (gtk_builder_get_object (builder, "hour-spin"));
	dialog->min_spin = GTK_WIDGET (gtk_builder_get_object (builder, "minute-spin"));
	dialog->sec_spin = GTK_WIDGET (gtk_builder_get_object (builder, "second-spin"));
	
	// REPEAT SETTINGS
	dialog->repeat_expand = GTK_WIDGET (gtk_builder_get_object (builder, "repeat-expand"));
	dialog->repeat_label  = GTK_WIDGET (gtk_builder_get_object (builder, "repeat-label"));
	
	// The check buttons have the same name as the 3 letter
	// string representation of the day.
	for (r = ALARM_REPEAT_SUN, i = 0; r <= ALARM_REPEAT_SAT; r = 1 << ++i) {
		dialog->repeat_check[i] = GTK_WIDGET (gtk_builder_get_object (builder, alarm_repeat_to_string (r)));
	}
	
	// SNOOZE SETTINGS
	dialog->snooze_check = GTK_WIDGET (gtk_builder_get_object (builder, "snooze-check"));
	dialog->snooze_spin  = GTK_WIDGET (gtk_builder_get_object (builder, "snooze-spin"));
	
	snooze_label = gtk_bin_get_child (GTK_BIN (dialog->snooze_check));
	g_object_set (G_OBJECT (snooze_label), "use-markup", TRUE, NULL);
	
	// NOTIFY SETTINGS
	dialog->notify_sound_radio       = GTK_WIDGET (gtk_builder_get_object (builder, "sound-radio"));
	dialog->notify_sound_box         = GTK_WIDGET (gtk_builder_get_object (builder, "sound-box"));
	dialog->notify_sound_combo       = GTK_WIDGET (gtk_builder_get_object (builder, "sound-combo"));
	dialog->notify_sound_preview     = GTK_WIDGET (gtk_builder_get_object (builder, "sound-play"));
	dialog->notify_sound_loop_check  = GTK_WIDGET (gtk_builder_get_object (builder, "sound-loop-check"));
	dialog->notify_app_radio         = GTK_WIDGET (gtk_builder_get_object (builder, "app-radio"));
	dialog->notify_app_box           = GTK_WIDGET (gtk_builder_get_object (builder, "app-box"));
	dialog->notify_app_combo         = GTK_WIDGET (gtk_builder_get_object (builder, "app-combo"));
	dialog->notify_app_command_box   = GTK_WIDGET (gtk_builder_get_object (builder, "app-command-box"));
	dialog->notify_app_command_entry = GTK_WIDGET (gtk_builder_get_object (builder, "app-command-entry"));
	
	
	// Load apps list
	alarm_applet_apps_load (applet);
	
	return dialog;
}

void
alarm_settings_dialog_show (AlarmSettingsDialog *dialog, Alarm *alarm)
{
    alarm_settings_dialog_set_alarm (dialog, alarm);

    gtk_widget_show_all (dialog->dialog);
	gtk_window_present (GTK_WINDOW (dialog->dialog));
}
