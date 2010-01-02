/*
 * prefs.c -- Alarm Clock global preferences
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
preferences_dialog_display (AlarmApplet *applet)
{
	if (applet->preferences_dialog != NULL) {
		// Dialog already open.
		gtk_window_present (GTK_WINDOW (applet->preferences_dialog));
		return;
	}
	
    GtkBuilder *builder = applet->ui;
	
	// Fetch widgets
	applet->preferences_dialog            = GTK_DIALOG (gtk_builder_get_object(builder, "preferences"));
	//applet->pref_label_show               = GTK_WIDGET (gtk_builder_get_object(builder, "show-label"));
	//applet->pref_label_type_box           = GTK_WIDGET (gtk_builder_get_object(builder, "label-type-box"));
	//applet->pref_label_type_time          = GTK_WIDGET (gtk_builder_get_object(builder, "time-radio"));
	//applet->pref_label_type_remain        = GTK_WIDGET (gtk_builder_get_object(builder, "remain-radio"));
	
	// Update UI
	//pref_update_label_show (applet);
	//pref_update_label_type (applet);
	
	// Set response and connect signal handlers
	
	//g_signal_connect (applet->preferences_dialog, "response", 
	//				  G_CALLBACK (preferences_dialog_response_cb), applet);
	
	// Display
	/*g_signal_connect (applet->pref_label_show, "toggled",
					  G_CALLBACK (pref_label_show_changed_cb), applet);
	
	g_signal_connect (applet->pref_label_type_time, "toggled",
					  G_CALLBACK (pref_label_type_changed_cb), applet);
	
	g_signal_connect (applet->pref_label_type_remain, "toggled",
					  G_CALLBACK (pref_label_type_changed_cb), applet);
	*/
	// Show it all
	//gtk_widget_show_all (GTK_WIDGET (applet->preferences_dialog));
    gtk_dialog_run (GTK_DIALOG (applet->preferences_dialog));
}
