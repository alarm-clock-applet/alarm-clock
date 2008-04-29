#include <time.h>

#include "alarm-applet.h"
#include "ui.h"
#include "alarms-list.h"

enum
{
    PIXBUF_COL,
    TEXT_COL,
    N_COLUMNS
};

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
		g_object_set (applet->label, "label", ALARM_DEF_LABEL, NULL);
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
	
	g_object_set(applet->label, "label", tmp, NULL);
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
	
	gtk_widget_set_tooltip_markup (GTK_WIDGET (applet->parent), tip->str);
	
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
	
	gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo_box));

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
	
	if (event->type == GDK_2BUTTON_PRESS || event->type == GDK_3BUTTON_PRESS) {
		/* Double click: Open list alarms */
		list_alarms_dialog_display (applet);
	} else {
		alarm_applet_snooze_alarms (applet);
	}
	
	/* Show edit alarms dialog */
	//display_list_alarms_dialog (applet);
	
	return TRUE;
}

/* Taken from the GeyesApplet */
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

/* Taken from gnome-panel/button-widget.c */
static void
load_icon (AlarmApplet *applet)
{
	GtkWidget *w;
	GdkPixbuf *icon;
	gint	  size, pbsize;
	
	w = GTK_WIDGET (applet->parent);
	
	if (panel_applet_get_orient (applet->parent) == PANEL_APPLET_ORIENT_UP ||
		panel_applet_get_orient (applet->parent) == PANEL_APPLET_ORIENT_DOWN)
		size = w->allocation.height;
	else
		size = w->allocation.width;
	
	if (size < 22)
		size = 16;
	else if (size < 24)
		size = 22;
	else if (size < 32)
		size = 24;
	else if (size < 48)
		size = 32;
	else if (size < 64)
		size = 48;
	else
		size = 64;

	// Reload icon only if the size is different.}
	icon = gtk_image_get_pixbuf (GTK_IMAGE (applet->icon));
	
	if (icon) {
		if (IS_HORIZONTAL (applet->parent))
			pbsize = gdk_pixbuf_get_height (icon);
		else
			pbsize = gdk_pixbuf_get_width (icon);
		
		if (pbsize == size) {
			// Do nothing
			//g_debug ("load_icon: Existing size the same.");
			return;
		}
	}
	
	g_debug ("Resizing icon to %dx%d...", size, size);
	
	icon = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
									 ALARM_ICON,
									 size,
									 0, NULL);
	
	if (icon == NULL) {
		g_critical ("Icon not found.");
		return;
	}
	
	g_object_set (applet->icon, "pixbuf", icon, NULL);
	
	if (icon)
		g_object_unref (icon);
}

static void
change_size_cb (GtkWidget 	  *widget,
				GtkAllocation *alloc,
				AlarmApplet	  *applet) 
{	
	load_icon (applet);
}



static void
update_orient (AlarmApplet *applet)
{
	const gchar *text;
	int          min_width;
	gdouble      new_angle;
	gdouble      angle;
	gint		 width;
	GtkWidget	*box;
	
	if (IS_HORIZONTAL (applet->parent) && ORIENT_IS_HORIZONTAL (applet->orient)) {
		// Nothing to do, both old and new orientation is horizontal.
		return;
	}
	
	/* Do we need to repack? */
	if (IS_HORIZONTAL (applet->parent) != ORIENT_IS_HORIZONTAL (applet->orient)) {
		g_debug ("update_orient: REPACK");
		
		// Add new
		if (IS_HORIZONTAL (applet->parent))
			box = gtk_hbox_new(FALSE, 6);
		else
			box = gtk_vbox_new(FALSE, 6);
		
		/* Keep children */
		g_object_ref (applet->icon);
		g_object_ref (applet->label);
		
		/* Remove old */
		gtk_container_remove (GTK_CONTAINER (applet->box), applet->icon);
		gtk_container_remove (GTK_CONTAINER (applet->box), applet->label);
		gtk_container_remove (GTK_CONTAINER (applet->parent), applet->box);
		
		/* Pack */
		gtk_box_pack_start_defaults(GTK_BOX (box), applet->icon);
		gtk_box_pack_start_defaults(GTK_BOX (box), applet->label);
		
		applet->box = box;
		
		/* Add to container and show */
		gtk_container_add (GTK_CONTAINER (applet->parent), box);
		gtk_widget_show_all (GTK_WIDGET (applet->parent));
	}
	
	switch (panel_applet_get_orient (applet->parent)) {
	case PANEL_APPLET_ORIENT_LEFT:
		new_angle = 270;
		break;
	case PANEL_APPLET_ORIENT_RIGHT:
		new_angle = 90;
		break;
	default:
		new_angle = 0;
		break;
	}
	
	
	angle = gtk_label_get_angle (GTK_LABEL (applet->label));
	if (angle != new_angle) {
		//unfix_size (cd);
		gtk_label_set_angle (GTK_LABEL (applet->label), new_angle);
	}
}

static void
orient_change_cb (PanelApplet *a,
					  PanelAppletOrient orient,
					  AlarmApplet *applet)
{
	g_debug ("applet_orient_change");
	
	update_orient (applet);
	
	// Store new orientation
	applet->orient = panel_applet_get_orient (applet->parent);
}


static void
unrealize_cb (GtkWidget *object, AlarmApplet *applet)
{
	alarm_applet_destroy (applet);
}

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

  	/*icon = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
									 ALARM_ICON,
									 48,
									 GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
	
	if (icon == NULL) {
		g_critical ("Icon not found.");
	}*/
	
	message = alarm->message;
	
	applet->notify = notify_notification_new (_("Alarm!"), message, icon, GTK_WIDGET (applet->icon));

	/* XXX: it would be nice to pass this as a named icon */
	//notify_notification_set_icon_from_pixbuf (applet->notify, icon);
	//gdk_pixbuf_unref (icon);

	result = notify_notification_show (applet->notify, &error);
	
	if (error)
	{
	   g_warning (error->message);
	   g_error_free (error);
	}

	//g_object_unref (G_OBJECT (n));

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
	
	/* Set up PanelApplet signals */
	g_signal_connect (G_OBJECT(applet->parent), "button-press-event",
					  G_CALLBACK(button_cb), applet);
	
	g_signal_connect (G_OBJECT(applet->parent), "unrealize",
					  G_CALLBACK(unrealize_cb), applet);
	
	g_signal_connect (G_OBJECT(applet->parent), "change-background",
					  G_CALLBACK (applet_back_change), applet);
	
	g_signal_connect (G_OBJECT(applet->parent), "change-orient",
					  G_CALLBACK (orient_change_cb), applet);
	
	g_signal_connect (G_OBJECT(applet->parent), "size-allocate",
					  G_CALLBACK (change_size_cb), applet);
	
	/* Set up container box */
	if (IS_HORIZONTAL (applet->parent))
		applet->box = gtk_hbox_new(FALSE, 6);
	else
		applet->box = gtk_vbox_new(FALSE, 6);
	
	/* Store orientation for future reference */
	applet->orient = panel_applet_get_orient (applet->parent);
	
	/* Set up icon and label */
	applet->icon = gtk_image_new ();
	load_icon (applet);
	
	applet->label = g_object_new(GTK_TYPE_LABEL,
								 "label", ALARM_DEF_LABEL,
								 "use-markup", TRUE,
								 "visible", applet->show_label,
								 "no-show-all", TRUE,			/* So gtk_widget_show_all() won't set visible to TRUE */
								 NULL);
	
	/* Set up UI updater */
	applet->timer_id = g_timeout_add_seconds (1, (GSourceFunc)alarm_applet_ui_update, applet);
	
	/* Pack */
	gtk_box_pack_start_defaults(GTK_BOX (applet->box), applet->icon);
	gtk_box_pack_start_defaults(GTK_BOX (applet->box), applet->label);
	
	/* Update orientation */
	update_orient (applet);
	
	/* Add to container and show */
	gtk_container_add (GTK_CONTAINER (applet->parent), applet->box);
	gtk_widget_show_all (GTK_WIDGET (applet->parent));
	
	alarm_applet_update_tooltip (applet);
}





static void
menu_snooze_alarm_cb (BonoboUIComponent *component,
					 AlarmApplet *applet,
					 const gchar *cname)
{
	g_debug("menu_snooze_alarm");
	
	alarm_applet_snooze_alarms (applet);
}

static void
menu_clear_alarm_cb (BonoboUIComponent *component,
					 AlarmApplet *applet,
					 const gchar *cname)
{
	g_debug("menu_clear_alarm");
	
	alarm_applet_clear_alarms (applet);
}

static void
menu_list_alarms_cb (BonoboUIComponent *component,
					 gpointer data,
					 const gchar *cname)
{
	AlarmApplet *applet = (AlarmApplet *)data;
	list_alarms_dialog_display (applet);
}

static void
menu_preferences_cb (BonoboUIComponent *component,
					 AlarmApplet *applet,
					 const gchar *cname)
{
	/* Construct the preferences dialog and show it here */
	g_debug("preferences_dialog");
	
	preferences_dialog_display (applet);
}

static void
menu_about_cb (BonoboUIComponent *component,
			   AlarmApplet *applet,
			   const gchar *cname)
{
	/* Construct the about dialog and show it here */
	g_debug("about_dialog");
	
	static const gchar *const authors[] = {
            "Johannes H. Jensen <joh@deworks.net>",
            NULL
    };
    static const gchar *const documenters[] = {
            "Johannes H. Jensen <joh@deworks.net>",
            NULL
    };
    static const gchar *const artists[] = {
            "Lasse Gulvåg Sætre <lassegs@gmail.com>",
            NULL
    };

    gtk_show_about_dialog (NULL,
    					   "program-name",	ALARM_NAME,
    					   "title", 		_("About " ALARM_NAME),
                           "version",       VERSION,
                           "copyright",     "\xC2\xA9 2007 Johannes H. Jensen",
                           "website",		"http://alarm-clock.pseudoberries.com/",
                           "authors",       authors,
                           "documenters",   documenters,
                           "artists",       artists, 
                           "logo-icon-name",ALARM_ICON,
                           NULL);
}

/*
 * Set up menu
 */
void
alarm_applet_menu_init (AlarmApplet *applet)
{
	static const gchar *menu_xml =
		"<popup name=\"button3\">\n"
		"   <menuitem name=\"Snooze Item\" "
		"			  verb=\"SnoozeAlarm\" "
		"			_label=\"_Snooze alarms\" "
		"		   pixtype=\"stock\" "
		"		   pixname=\"gtk-clear\"/>\n"
		"   <menuitem name=\"Clear Item\" "
		"			  verb=\"ClearAlarm\" "
		"			_label=\"_Clear alarms\" "
		"		   pixtype=\"stock\" "
		"		   pixname=\"gtk-clear\"/>\n"
		"   <separator/>\n"
		"   <menuitem name=\"List Alarms\" "
		"			  verb=\"ListAlarms\" "
		"			_label=\"_Edit Alarms\" "
		"		   pixtype=\"stock\" "
		"		   pixname=\"gtk-edit\"/>\n"
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
			BONOBO_UI_VERB ("SnoozeAlarm", menu_snooze_alarm_cb),
			BONOBO_UI_VERB ("ClearAlarm", menu_clear_alarm_cb),
			BONOBO_UI_VERB ("ListAlarms", menu_list_alarms_cb),
			BONOBO_UI_VERB ("Preferences", menu_preferences_cb),
			BONOBO_UI_VERB ("About", menu_about_cb),
			BONOBO_UI_VERB_END
	};
	
	panel_applet_setup_menu (PANEL_APPLET (applet->parent),
	                         menu_xml,
	                         menu_verbs,
	                         applet);
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


