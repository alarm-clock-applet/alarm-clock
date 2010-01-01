/*
 * ui.c - Alarm Clock applet UI routines
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

#include <time.h>
#include <string.h>
#ifdef HAVE_LIBNOTIFY
#include <libnotify/notify.h>
#endif

#include "alarm-applet.h"
#include "ui.h"

enum
{
    GICON_COL,
    TEXT_COL,
    N_COLUMNS
};

/*
 * Load a user interface by name
 */
GtkBuilder *
alarm_applet_ui_load (const char *name, AlarmApplet *applet)
{
    GtkBuilder *builder = NULL;
    GError *error = NULL;
    char *filename;
   
    filename = alarm_applet_get_data_path (name);

    g_assert(filename != NULL);
    
    builder = gtk_builder_new();
    
    if (gtk_builder_add_from_file (builder, filename, &error)) {
        /* Connect signals */
        gtk_builder_connect_signals (builder, applet);
    } else {
        g_critical(_("Couldn't load the interface '%s'. %s"), filename, error->message);
        g_error_free (error);
    }

    g_free (filename);

    return builder;
}

void
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


// TODO: Should display any triggered alarms / etc.
void
alarm_applet_label_update (AlarmApplet *applet)
{
	Alarm *a;
	struct tm *tm;
	guint hour, min, sec, now;
	gchar *tmp;
	
	if (!applet->upcoming_alarm) {
		// No upcoming alarms
//		g_object_set (applet->label, "label", ALARM_DEF_LABEL, NULL);
		return;
	}
	
	a = applet->upcoming_alarm;
	
	if (applet->label_type == LABEL_TYPE_REMAIN) {
		/* Remaining time */
		tm = alarm_get_remain (a);
	} else {
		/* Alarm time */
		tm = alarm_get_time (a);
	}
	
	tmp = g_strdup_printf(_("%02d:%02d:%02d"), tm->tm_hour, tm->tm_min, tm->tm_sec);
	
//	g_object_set(applet->label, "label", tmp, NULL);
	g_free(tmp);
}

// TODO: Refactor for more fancy tooltip with alarm summary.
void
alarm_applet_update_tooltip (AlarmApplet *applet)
{
	struct tm *time, *remain;
	GList *l;
	Alarm *a;
	GString *tip;
	guint count = 0;

	tip = g_string_new ("");
	
	// Find all active alarms
	for (l = applet->alarms; l; l = l->next) {
		a = ALARM (l->data);
		
		if (!a->active) continue;
		
		count++;
		
		time   = alarm_get_time (a);
		remain = alarm_get_remain (a);
		
		g_string_append_printf (tip, _("\n(%c) <b>%s</b> @%02d:%02d:%02d (-%02d:%02d:%02d)"), (a->type == ALARM_TYPE_TIMER) ? 'T' : 'A', a->message,
														time->tm_hour, time->tm_min, time->tm_sec, remain->tm_hour, remain->tm_min, remain->tm_sec);
	}
	
	if (count > 0) {
		tip = g_string_prepend (tip, _("Active alarms:"));
	} else {
		tip = g_string_append (tip, _("No active alarms"));
	}
	
	tip = g_string_append (tip, _("\n\nClick to snooze alarms"));
	tip = g_string_append (tip, _("\nDouble click to edit alarms"));
	
	//gtk_widget_set_tooltip_markup (GTK_WIDGET (applet->parent), tip->str);
	
	g_string_free (tip, TRUE);
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
void
fill_combo_box (GtkComboBox *combo_box, GList *list, const gchar *custom_label)
{
	GList *l;
	GtkTreeModel *model;
	GtkCellRenderer *renderer;
	GtkTreeIter iter;
	AlarmListEntry *entry;
	
	g_debug ("fill_combo_box... %d", g_list_length (list));

	gtk_combo_box_set_row_separator_func (combo_box, is_separator,
					  GINT_TO_POINTER (g_list_length (list)), NULL);

	model = GTK_TREE_MODEL (gtk_list_store_new (2, G_TYPE_ICON, G_TYPE_STRING));
	gtk_combo_box_set_model (combo_box, model);
	
	gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo_box));

	renderer = gtk_cell_renderer_pixbuf_new ();

	/* not all cells have a pixbuf, this prevents the combo box to shrink */
	gtk_cell_renderer_set_fixed_size (renderer, -1, 22);
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), renderer, FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), renderer,
					"gicon", GICON_COL,
					NULL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), renderer,
					"text", TEXT_COL,
					NULL);

	for (l = list; l != NULL; l = g_list_next (l)) {
		GIcon *icon;
		
		entry = (AlarmListEntry *) l->data;
		icon = g_icon_new_for_string (entry->icon, NULL);

		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
							GICON_COL, icon,
							TEXT_COL, entry->name,
							-1);

		g_object_unref (icon);
	}

	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, -1);
	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			GICON_COL, NULL,
			TEXT_COL, custom_label,
			-1);
}


static gboolean
button_cb (GtkStatusIcon  *status_icon,
           GdkEventButton *event,
           gpointer       data)   
{
	AlarmApplet *applet = (AlarmApplet *)data;
	
	g_debug("BUTTON: %d", event->button);
	
	/* React only to left mouse button */
	if (event->button == 2 || event->button == 3) {
		return FALSE;
	}
	
	if (event->type == GDK_2BUTTON_PRESS || event->type == GDK_3BUTTON_PRESS) {
		/* Double click: Open list alarms */
        alarm_list_window_show (applet->list_window);
	} else {
		alarm_applet_snooze_alarms (applet);
	}
	
	/* Show edit alarms dialog */
	//display_list_alarms_dialog (applet);
	
	return TRUE;
}


#ifdef HAVE_LIBNOTIFY
static void
alarm_applet_notification_action_cb (NotifyNotification *notify,
									 gchar *action,
									 gpointer data)
{
	Alarm *alarm = ALARM (data);
	
	g_debug ("NOTIFY ACTION: %s", action);
	
	if (strcmp (action, "clear") == 0) {
		g_debug ("NOTIFY CLEAR #%d", alarm->id);
		alarm_clear (alarm);
	} else {
		// "snooze"
		g_debug ("NOTIFY SNOOZE #%d", alarm->id);
		alarm_snooze (alarm);
	}
}
#endif

/* Taken from the battery applet */
gboolean
alarm_applet_notification_display (AlarmApplet *applet, Alarm *alarm)
{
#ifdef HAVE_LIBNOTIFY
	GError *error = NULL;
	//GdkPixbuf *icon;
	gboolean result;
	const gchar *message;
	const gchar *icon = (alarm->type == ALARM_TYPE_CLOCK) ? ALARM_ICON : TIMER_ICON;
	
	if (!notify_is_initted () && !notify_init (_("Alarm Applet")))
		return FALSE;
	
	message = alarm->message;
	
//	applet->notify = notify_notification_new (_("Alarm!"), message, icon, GTK_WIDGET (applet->icon));
	
	notify_notification_set_timeout (applet->notify, NOTIFY_EXPIRES_NEVER);
	notify_notification_add_action (applet->notify, "clear", "Clear", alarm_applet_notification_action_cb, alarm, NULL);
	
	if (alarm->snooze) {
		notify_notification_add_action (applet->notify, "snooze", "Snooze", alarm_applet_notification_action_cb, alarm, NULL);
	}

	result = notify_notification_show (applet->notify, &error);
	
	if (error)
	{
	   g_warning (error->message);
	   g_error_free (error);
	}

	return result;
#else
	return FALSE;
#endif
}

gboolean
alarm_applet_notification_close (AlarmApplet *applet)
{
#ifdef HAVE_LIBNOTIFY
	gboolean result;
	GError *error = NULL;
	
	result = notify_notification_close (applet->notify, &error);
	
	if (error)
	{
	   g_warning (error->message);
	   g_error_free (error);
	}
	
	g_object_unref (applet->notify);
	applet->notify = NULL;
#endif
}

/*
 * Updates label etc
 */
static gboolean
alarm_applet_ui_update (AlarmApplet *applet)
{
	if (applet->show_label) {
		alarm_applet_label_update (applet);
	}
	
	alarm_applet_update_tooltip (applet);
	
	return TRUE;
}






void
alarm_applet_ui_init (AlarmApplet *applet)
{
	GtkWidget *hbox;
	GdkPixbuf *icon;
    
    GError* error = NULL;
    char *filename;

    /* Load UI with GtkBuilder */
    applet->ui = alarm_applet_ui_load ("alarm-clock.ui", applet);
    
    /* Initialize status icon */
    alarm_applet_status_init(applet);

	/* Initialize alarm list window */
	applet->list_window = alarm_list_window_new (applet);

    /* Initialize alarm settings dialog */
    applet->settings_dialog = alarm_settings_dialog_new (applet);
    
    
    /* Connect signals */
    //gtk_builder_connect_signals (applet->ui, applet);
    

	
	
	//g_object_set(applet->status_icon, "blinking", TRUE, NULL);

	return;
	
	/* Set up PanelApplet signals */
	/*g_signal_connect (G_OBJECT(applet->parent), "button-press-event",
					  G_CALLBACK(button_cb), applet);
	
	g_signal_connect (G_OBJECT(applet->parent), "unrealize",
					  G_CALLBACK(unrealize_cb), applet);
	
	g_signal_connect (G_OBJECT(applet->parent), "change-background",
					  G_CALLBACK (applet_back_change), applet);
	
	g_signal_connect (G_OBJECT(applet->parent), "change-orient",
					  G_CALLBACK (orient_change_cb), applet);
	
	g_signal_connect (G_OBJECT(applet->parent), "size-allocate",
					  G_CALLBACK (change_size_cb), applet);
	*/
	
	/* Set up container box */
	/*if (IS_HORIZONTAL (applet->parent))
		applet->box = gtk_hbox_new(FALSE, 6);
	else
		applet->box = gtk_vbox_new(FALSE, 6);
	*/
	/* Store orientation for future reference */
	//applet->orient = panel_applet_get_orient (applet->parent);
	
	/* Set up icon and label */
	//applet->icon = gtk_image_new ();
	//alarm_applet_icon_update (applet);
	
	/*applet->label = g_object_new(GTK_TYPE_LABEL,
								 "label", ALARM_DEF_LABEL,
								 "use-markup", TRUE,
								 "visible", applet->show_label,
								 "no-show-all", TRUE,			/* So gtk_widget_show_all() won't set visible to TRUE */
	/*							 NULL);*/
	
	/* Set up UI updater */
//	alarm_applet_ui_update (applet);
//	applet->timer_id = g_timeout_add_seconds (1, (GSourceFunc)alarm_applet_ui_update, applet);
	
	/* Pack */
//	gtk_box_pack_start_defaults(GTK_BOX (applet->box), applet->icon);
//	gtk_box_pack_start_defaults(GTK_BOX (applet->box), applet->label);
	
	/* Update orientation */
//	update_orient (applet);
	
	/* Add to container and show */
	//gtk_container_add (GTK_CONTAINER (applet->parent), applet->box);
	//gtk_widget_show_all (GTK_WIDGET (applet->parent));
	
//	alarm_applet_update_tooltip (applet);
}

/*
 * Initialize status icon
 */
void
alarm_applet_status_init (AlarmApplet *applet)
{
    applet->status_icon = GTK_STATUS_ICON (gtk_builder_get_object (applet->ui, "status_icon"));
    applet->status_menu = GTK_WIDGET (gtk_builder_get_object (applet->ui, "status_menu"));

    gtk_status_icon_set_visible (applet->status_icon, TRUE);
}

/*
 * Menu callbacks:
 */

G_MODULE_EXPORT void
alarm_applet_status_activate (GtkStatusIcon *status_icon,
							  gpointer       user_data)
{
	AlarmApplet *applet = (AlarmApplet *)user_data;

	g_debug("alarm_applet_status_activate()");

	// TODO: Snooze alarms if any
	
	alarm_list_window_toggle (applet->list_window);
}

G_MODULE_EXPORT void
alarm_applet_status_popup (GtkStatusIcon  *status_icon,
                           guint           button,
                           guint           activate_time,
                           gpointer        user_data)
{
    AlarmApplet *applet = (AlarmApplet *)user_data;

    gtk_menu_popup (GTK_MENU (applet->status_menu),
                    NULL,
                    NULL,
                    gtk_status_icon_position_menu,
                    status_icon,
                    button,
                    activate_time);
}

G_MODULE_EXPORT void
status_menu_snooze_cb (GtkMenuItem *menuitem,
                       gpointer     user_data)
{
    AlarmApplet *applet = (AlarmApplet *)user_data;
    
	alarm_applet_snooze_alarms (applet);
}

G_MODULE_EXPORT void
status_menu_clear_cb (GtkMenuItem *menuitem,
                      gpointer     user_data)
{
    AlarmApplet *applet = (AlarmApplet *)user_data;
    
	alarm_applet_clear_alarms (applet);
}


G_MODULE_EXPORT void
status_menu_edit_cb (GtkMenuItem *menuitem,
                     gpointer     user_data)
{
    AlarmApplet *applet = (AlarmApplet *)user_data;
	alarm_list_window_show (applet->list_window);
}


G_MODULE_EXPORT void
status_menu_prefs_cb (GtkMenuItem *menuitem,
                      gpointer     user_data)
{
    AlarmApplet *applet = (AlarmApplet *)user_data;
	
	preferences_dialog_display (applet);
}

G_MODULE_EXPORT void
status_menu_about_cb (GtkMenuItem *menuitem,
                      gpointer     user_data)
{
    AlarmApplet *applet = (AlarmApplet *)user_data;
    
    gboolean visible;	
    GtkAboutDialog *dialog = GTK_ABOUT_DIALOG (
        gtk_builder_get_object (applet->ui, "about-dialog"));
    
    g_object_get (dialog, "visible", &visible, NULL);

    if (!visible) {
        // Set properties and show
        g_object_set (G_OBJECT (dialog),
                      "program-name", ALARM_NAME,
                      "title", _("About " ALARM_NAME),
                      "version", VERSION,
                      NULL);

        gtk_dialog_run (dialog);
        
    } else {
        // Already visible, present it
        gtk_window_present (GTK_WINDOW (dialog));
    }
}

/*
 * An error callback for MediaPlayers
 */
void
media_player_error_cb (MediaPlayer *player, GError *err, GtkWindow *parent)
{
	gchar *uri, *tmp;
	
	uri = media_player_get_uri (player);
	tmp = g_strdup_printf (_("%s: %s"), uri, err->message);
	
	g_critical (_("Could not play '%s': %s"), uri, err->message);
	display_error_dialog (_("Could not play"), tmp, parent);
	
	g_free (tmp);
	g_free (uri);
}


