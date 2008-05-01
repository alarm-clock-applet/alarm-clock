/*
 * prefs.c -- Alarm Clock global preferences
 * 
 * Copyright (C) 2007-2008 Johannes H. Jensen <joh@deworks.net>
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
 * 		Johannes H. Jensen <joh@deworks.net>
 */

#include "prefs.h"

void
preferences_dialog_response_cb (GtkDialog *dialog,
								gint rid,
								AlarmApplet *applet)
{	
	g_debug ("Preferences Response: %s", (rid == GTK_RESPONSE_CLOSE) ? "Close" : "Unknown");
	
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
	case LABEL_TYPE_TIME:
		g_object_set (applet->pref_label_type_time, "active", TRUE, NULL);
		break;
	default:
		// LABEL_TYPE_TOTAL
		g_object_set (applet->pref_label_type_remain, "active", TRUE, NULL);
		break;
	}
}



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
	
	if (strcmp (name, "time-radio") == 0) {
		kval = gconf_enum_to_string (label_type_enum_map, LABEL_TYPE_TIME);
		panel_applet_gconf_set_string (applet->parent, KEY_LABEL_TYPE, kval, NULL);
	} else {
		kval = gconf_enum_to_string (label_type_enum_map, LABEL_TYPE_REMAIN);
		panel_applet_gconf_set_string (applet->parent, KEY_LABEL_TYPE, kval, NULL);
	}
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
	applet->pref_label_type_time          = glade_xml_get_widget (ui, "time-radio");
	applet->pref_label_type_remain        = glade_xml_get_widget (ui, "remain-radio");
	
	// Update UI
	pref_update_label_show (applet);
	pref_update_label_type (applet);
	
	// Set response and connect signal handlers
	
	g_signal_connect (applet->preferences_dialog, "response", 
					  G_CALLBACK (preferences_dialog_response_cb), applet);
	
	// Display
	g_signal_connect (applet->pref_label_show, "toggled",
					  G_CALLBACK (pref_label_show_changed_cb), applet);
	
	g_signal_connect (applet->pref_label_type_time, "toggled",
					  G_CALLBACK (pref_label_type_changed_cb), applet);
	
	g_signal_connect (applet->pref_label_type_remain, "toggled",
					  G_CALLBACK (pref_label_type_changed_cb), applet);
	
	// Show it all
	gtk_widget_show_all (GTK_WIDGET (applet->preferences_dialog));
}
