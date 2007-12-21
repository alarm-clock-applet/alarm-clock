#include "alarm-applet.h"
#include "ui.h"

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

void
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

gboolean
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
		
		pixbuf = gtk_icon_theme_load_icon (theme, entry->icon, 22, 0, NULL);
		
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
					PIXBUF_COL, pixbuf,
					TEXT_COL, entry->name,
					-1);
		
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

void
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
		
		alarm_gconf_set_started (applet, TRUE);
		alarm_gconf_set_alarm (applet, hour, minute, second);
		alarm_gconf_set_message (applet, message);
		
		g_free (message);
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	applet->set_alarm_dialog = NULL;
}

void
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

void
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
applet_back_change (PanelApplet			*a,
					PanelAppletBackgroundType	type,
					GdkColor			*color,
					GdkPixmap			*pixmap,
					AlarmApplet			*applet) 
{
        /* taken from the TrashApplet */
        GtkRcStyle *rc_style;
        GtkStyle *style;

        /* reset style */
        gtk_widget_set_style (GTK_WIDGET (applet->parent), NULL);
        rc_style = gtk_rc_style_new ();
        gtk_widget_modify_style (GTK_WIDGET (applet->parent), rc_style);
        gtk_rc_style_unref (rc_style);

        switch (type) {
                case PANEL_COLOR_BACKGROUND:
                        gtk_widget_modify_bg (GTK_WIDGET (applet->parent),
                                        GTK_STATE_NORMAL, color);
                        break;

                case PANEL_PIXMAP_BACKGROUND:
                        style = gtk_style_copy (GTK_WIDGET (
                        		applet->parent)->style);
                        if (style->bg_pixmap[GTK_STATE_NORMAL])
                                g_object_unref
                                        (style->bg_pixmap[GTK_STATE_NORMAL]);
                        style->bg_pixmap[GTK_STATE_NORMAL] = g_object_ref
                                (pixmap);
                        gtk_widget_set_style (GTK_WIDGET (applet->parent),
                                        style);
                        g_object_unref (style);
                        break;

                case PANEL_NO_BACKGROUND:
                default:
                        break;
        }

}

static void
destroy_cb (GtkObject *object, AlarmApplet *applet)
{
	if (applet->sounds != NULL) {
		alarm_list_entry_list_free(&(applet->sounds));
	}

	timer_remove (applet);
}

void
ui_setup (AlarmApplet *applet)
{
	GtkWidget *hbox;
	GdkPixbuf *icon;
	
	g_signal_connect (G_OBJECT(applet->parent), "button-press-event",
					  G_CALLBACK(button_cb), applet);
	
	g_signal_connect (G_OBJECT(applet->parent), "destroy",
					  G_CALLBACK(destroy_cb), applet);
	
	g_signal_connect (G_OBJECT(applet->parent), "change_background",
					  G_CALLBACK (applet_back_change), applet);
	
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

void
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