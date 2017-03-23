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
set_repeat_toggle_buttons(AlarmSettingsDialog *dialog, AlarmRepeat repeat);

void
alarm_settings_repeat_all (GtkButton *button, gpointer data);

void
alarm_settings_repeat_weekday (GtkButton *button, gpointer data);

void
alarm_settings_repeat_weekend (GtkButton *button, gpointer data);

void
alarm_settings_repeat_clear (GtkButton *button, gpointer data);

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

void
alarm_settings_changed_sound (GtkComboBox *combo, gpointer data);

void
alarm_settings_changed_app (GtkComboBox *combo, gpointer data);



#define REPEAT_LABEL	_("_Repeat: %s")



/*
 * Utility functions for updating various parts of the settings dialog.
 */

static void
alarm_settings_update_type (AlarmSettingsDialog *dialog)
{
    
    AlarmType type = dialog->alarm->type;
    gboolean clock_toggled = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->clock_toggle));
    gboolean timer_toggled = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->timer_toggle));

    if ((type == ALARM_TYPE_CLOCK && clock_toggled) ||
    	(type == ALARM_TYPE_TIMER && timer_toggled)) {
    	// No change
    	return;
    }

    g_debug ("AlarmSettingsDialog: update_type()");

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->clock_toggle),
		type == ALARM_TYPE_CLOCK);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->timer_toggle),
		type == ALARM_TYPE_TIMER);

	if (type == ALARM_TYPE_CLOCK) {
		gtk_widget_set_sensitive(dialog->repeat_expand, TRUE);
	} else {
		gtk_widget_set_sensitive(dialog->repeat_expand, FALSE);
	}
}

static void
alarm_settings_update_label (AlarmSettingsDialog *dialog)
{
	const gchar *entry_text = gtk_entry_get_text (GTK_ENTRY (dialog->label_entry));
    if (g_strcmp0 (entry_text, dialog->alarm->message) == 0) {
    	// No change
    	return;
    }

    g_debug ("AlarmSettingsDialog: update_label()");
    
    g_object_set (dialog->label_entry, "text", dialog->alarm->message, NULL);
}

static void
alarm_settings_update_time (AlarmSettingsDialog *dialog)
{
	struct tm *tm = alarm_get_time(dialog->alarm);
	gint hour = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dialog->hour_spin));
	gint min = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dialog->min_spin));
	gint sec = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dialog->sec_spin));

	if (tm->tm_hour == hour && tm->tm_min == min && tm->tm_sec == sec) {
		// No change
		return;
	}

    g_debug ("AlarmSettingsDialog: update_time() to %d:%d:%d", tm->tm_hour, tm->tm_min, tm->tm_sec);
	
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
	gchar *label, *rep;

    g_debug ("AlarmSettingsDialog: update_repeat()");
	
	/*
	 * Update check boxes
	 */
	for (r = ALARM_REPEAT_SUN, i = 0; r <= ALARM_REPEAT_SAT; r = 1 << ++i) {
		check = (dialog->alarm->repeat & r) != 0;
		
		// Activate the appropriate widget
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (dialog->repeat_check[i])) != check) {
			g_object_set (dialog->repeat_check[i], "active", check, NULL);
		}
	}
	

    /*
     * Update fancy expander label
     */
    rep = alarm_repeat_to_pretty (dialog->alarm->repeat);
    label = g_strdup_printf (REPEAT_LABEL, rep);
    g_object_set (dialog->repeat_label, "label", label, NULL);
    g_free (label);
    g_free (rep);
}

static void
alarm_settings_update_notify_type (AlarmSettingsDialog *dialog)
{
    AlarmNotifyType type = dialog->alarm->notify_type;
    
    gboolean sound_active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->notify_app_radio));
    gboolean sound_sensitive = gtk_widget_is_sensitive (dialog->notify_sound_box);
    gboolean app_active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->notify_app_radio));
    gboolean app_sensitive = gtk_widget_is_sensitive (dialog->notify_app_box);

    if ((type == ALARM_NOTIFY_SOUND && sound_active && sound_sensitive && !app_sensitive) ||
    	(type == ALARM_NOTIFY_COMMAND && app_active && !sound_sensitive && app_sensitive)) {
    	// No change
    	return;
    }

    g_debug ("AlarmSettingsDialog: update_notify_type()");

	// Enable selected
	switch (dialog->alarm->notify_type) {
        case ALARM_NOTIFY_COMMAND:
            g_object_set (dialog->notify_app_radio, "active", TRUE, NULL);
            g_object_set (dialog->notify_app_box, "sensitive", TRUE, NULL);

            // Disable others
            g_object_set (dialog->notify_sound_box, "sensitive", FALSE, NULL);

            if (dialog->player && dialog->player->state == MEDIA_PLAYER_PLAYING) {
                // Stop preview player
                media_player_stop (dialog->player);
            }

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
	gint pos;

	pos = gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->notify_sound_combo));
	item = g_list_nth_data (dialog->applet->sounds, pos);

	if (item && g_strcmp0 (item->data, dialog->alarm->sound_file) == 0) {
		// No change
		return;
	}

    g_debug ("AlarmSettingsDialog: update_sound()");

	/* Fill sounds list */
	fill_combo_box (GTK_COMBO_BOX (dialog->notify_sound_combo),
					dialog->applet->sounds, _("Select sound file..."));
	
	// Look for the selected sound file
	for (l = dialog->applet->sounds, pos = 0; l != NULL; l = l->next, pos++) {
		item = (AlarmListEntry *)l->data;
		if (strcmp (item->data, dialog->alarm->sound_file) == 0) {
			// Match!
			gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->notify_sound_combo), pos);
			break;
		}
	}
}

static void
alarm_settings_update_sound_repeat (AlarmSettingsDialog *dialog)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->notify_sound_loop_check)) == dialog->alarm->sound_loop) {
		// No change
		return;
	}

    g_debug ("AlarmSettingsDialog: update_sound_repeat()");
    
	g_object_set (dialog->notify_sound_loop_check, 
        "active", dialog->alarm->sound_loop, NULL);
}

static void
alarm_settings_update_app (AlarmSettingsDialog *dialog)
{
	AlarmListEntry *item;
	GList *l;
	guint pos, len;
	gboolean custom = FALSE;

	pos = gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->notify_app_combo));
	item = g_list_nth_data (dialog->applet->apps, pos);

	if (item && g_strcmp0 (item->data, dialog->alarm->command) == 0) {
		// No change
		return;
	}

    g_debug ("AlarmSettingsDialog: update_app()");
	
//	g_debug ("alarm_settings_update_app (%p): app_combo: %p, applet: %p, apps: %p", dialog, dialog->notify_app_combo, dialog->applet, dialog->applet->apps);
//	g_debug ("alarm_settings_update_app setting entry to %s", dialog->alarm->command);
    
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
alarm_settings_update_app_command (AlarmSettingsDialog *dialog)
{
    g_debug ("AlarmSettingsDialog: update_app_command()");
    
    if (g_strcmp0 (dialog->alarm->command, gtk_entry_get_text (GTK_ENTRY (dialog->notify_app_command_entry))) == 0) {
    	// No change
    	return;
    }
    
    g_object_set (dialog->notify_app_command_entry, "text", dialog->alarm->command, NULL);
}

static void
alarm_settings_update (AlarmSettingsDialog *dialog)
{
	alarm_settings_update_type (dialog);
    alarm_settings_update_label (dialog);
	alarm_settings_update_time (dialog);
	alarm_settings_update_repeat (dialog);
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
    
    if (g_strcmp0 (name, "notify-type") == 0) {
        alarm_settings_update_notify_type (dialog);
    }

    if (g_strcmp0 (name, "sound-file") == 0) {
	    alarm_settings_update_sound (dialog);
    }

    if (g_strcmp0 (name, "sound-repeat") == 0) {
       	g_object_set (dialog->notify_sound_loop_check, "active", dialog->alarm->sound_loop, NULL);
    }

    if (g_strcmp0 (name, "command") == 0) {
	    alarm_settings_update_app (dialog);
	    alarm_settings_update_app_command (dialog);
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
    gboolean toggled = gtk_toggle_button_get_active(toggle);

    g_assert (dialog->alarm != NULL);
    g_debug ("alarm_settings_changed_type(%s) -> %s",
             gtk_buildable_get_name (GTK_BUILDABLE (toggle)),
             toggled ? "TRUE" : "FALSE");

    if (GTK_WIDGET (toggle) == dialog->clock_toggle) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->timer_toggle), !toggled);
        if (toggled) {
            g_debug ("alarm_settings_changed_type: clock toggled");
            g_object_set (dialog->alarm, "type", ALARM_TYPE_CLOCK, NULL);
            gtk_widget_set_sensitive(dialog->repeat_expand, TRUE);
        }
    } else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->clock_toggle), !toggled);
        if (toggled) {
            g_debug ("alarm_settings_changed_type: timer toggled");
            g_object_set (dialog->alarm, "type", ALARM_TYPE_TIMER, NULL);
            gtk_widget_set_sensitive(dialog->repeat_expand, FALSE);
        }
    }
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
	struct tm *tm;

    g_assert (dialog->alarm != NULL);
    
    tm = alarm_get_time(dialog->alarm);
    hour = tm->tm_hour;
    min = tm->tm_min;
    sec = tm->tm_sec;

    // Check which spin button emitted the signal
    if (GTK_WIDGET (spinbutton) == dialog->hour_spin) {
		hour = gtk_spin_button_get_value (GTK_SPIN_BUTTON (dialog->hour_spin));
    } else if (GTK_WIDGET (spinbutton) == dialog->min_spin) {
    	min = gtk_spin_button_get_value (GTK_SPIN_BUTTON (dialog->min_spin));
    } else if (GTK_WIDGET (spinbutton) == dialog->sec_spin) {
    	sec = gtk_spin_button_get_value (GTK_SPIN_BUTTON (dialog->sec_spin));
    }

	alarm_set_time (dialog->alarm, hour, min, sec);
}

/**
 * Use 2-digits in time spin buttons
 */
gboolean
alarm_settings_output_time (GtkSpinButton *spin, gpointer data)
{
    GtkAdjustment *adj;
    gchar *text;
    gint value;
    adj = gtk_spin_button_get_adjustment (spin);
    value = (gint)gtk_adjustment_get_value (adj);
    text = g_strdup_printf ("%02d", value);
    gtk_entry_set_text (GTK_ENTRY (spin), text);
    g_free (text);

    return TRUE; 
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
	name   = gtk_buildable_get_name (GTK_BUILDABLE (togglebutton));
	rep    = alarm_repeat_from_string (name);
	active = gtk_toggle_button_get_active (togglebutton);
	
	g_debug("Changed repeat on: %s, active: %d", name, active);

	if (active)
		// Add rep
		new_rep = dialog->alarm->repeat | rep;
	else
		// Remove rep
		new_rep = dialog->alarm->repeat & ~rep;
	
	g_object_set (dialog->alarm, "repeat", new_rep, NULL);
}


void
set_repeat_toggle_buttons(AlarmSettingsDialog *dialog, AlarmRepeat repeat)
{
    AlarmRepeat r;
    gint i;
    gboolean check;

    for (r = ALARM_REPEAT_SUN, i = 0; r <= ALARM_REPEAT_SAT; r = 1 << ++i) {
        check = (repeat & r) != 0;
        g_object_set (dialog->repeat_check[i], "active", check, NULL);
    }
}

void
alarm_settings_repeat_all (GtkButton *button, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
    AlarmSettingsDialog *dialog = applet->settings_dialog;

    g_assert (dialog->alarm != NULL);
    g_debug("Changed repeat to All");
 
    set_repeat_toggle_buttons(dialog, ALARM_REPEAT_ALL);
    g_object_set (dialog->alarm, "repeat", ALARM_REPEAT_ALL, NULL);
    
}

void
alarm_settings_repeat_weekday (GtkButton *button, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
    AlarmSettingsDialog *dialog = applet->settings_dialog;

    g_assert (dialog->alarm != NULL);
    g_debug("Changed repeat to Weekdays");

    set_repeat_toggle_buttons(dialog, ALARM_REPEAT_WEEKDAYS);
    g_object_set (dialog->alarm, "repeat", ALARM_REPEAT_WEEKDAYS, NULL);
}

void
alarm_settings_repeat_weekend (GtkButton *button, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
    AlarmSettingsDialog *dialog = applet->settings_dialog;

    g_assert (dialog->alarm != NULL);
    
    g_debug("Changed repeat to Weekends");

    set_repeat_toggle_buttons(dialog, ALARM_REPEAT_WEEKENDS);
    g_object_set (dialog->alarm, "repeat", ALARM_REPEAT_WEEKENDS, NULL);
}

void
alarm_settings_repeat_clear (GtkButton *button, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
    AlarmSettingsDialog *dialog = applet->settings_dialog;

    g_assert (dialog->alarm != NULL);
    g_debug("Changed repeat to None");

    set_repeat_toggle_buttons(dialog, ALARM_REPEAT_NONE);
    g_object_set (dialog->alarm, "repeat", ALARM_REPEAT_NONE, NULL);
}

void
alarm_settings_changed_notify_type (GtkToggleButton *togglebutton, gpointer data)
{
    AlarmApplet *applet = (AlarmApplet *)data;
	AlarmSettingsDialog *dialog = applet->settings_dialog;
    
	const gchar *name = gtk_buildable_get_name (GTK_BUILDABLE (togglebutton));;
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
	
	AlarmListEntry *item;
	guint current_index, len;
	
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

    g_debug("alarm_settings_changed_sound_repeat");

    g_object_set (dialog->alarm, "sound-repeat", gtk_toggle_button_get_active (togglebutton), NULL);

    if (dialog->player && dialog->player->state == MEDIA_PLAYER_PLAYING) {
        // Update preview player
        dialog->player->loop = gtk_toggle_button_get_active (togglebutton);
    }
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
	
	AlarmListEntry *item;
	guint current_index, len;
	
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
preview_player_state_cb (MediaPlayer *player, MediaPlayerState state, gpointer data)
{
	AlarmSettingsDialog *dialog = (AlarmSettingsDialog *)data;
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
		if (dialog->player == NULL) {
			dialog->player = media_player_new (dialog->alarm->sound_file,
											   dialog->alarm->sound_loop,
											   preview_player_state_cb, dialog,
											   media_player_error_cb, dialog->dialog);
			if (dialog->player == NULL) {
				// Unable to create player
				alarm_error_trigger (dialog->alarm, ALARM_ERROR_PLAY, 
					_("Could not create player! Please check your sound settings."));
				return;
			}
		}
	
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
	

    /* Enable the alarm when closing the dialog */
    alarm_enable (dialog->alarm);

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
