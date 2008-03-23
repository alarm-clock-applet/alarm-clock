#include "alarms-list.h"
#include "edit-alarm.h"

static void 
list_alarms_toggled_cb  (GtkCellRendererToggle *cell_renderer,
						 gchar                 *path,
						 gpointer               data)
{
	GtkListStore *model = GTK_LIST_STORE (data);
	GtkTreeIter iter;
	Alarm *alarm;
	
	gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (model), &iter, path);
	gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 0, &alarm, -1);
	
	g_debug ("list_alarms_toggled %s: #%d", path, alarm->id);
	
	if (!alarm->active && alarm->type == ALARM_TYPE_CLOCK) {
		// Update time
		g_debug ("\tUpdate TIME!");
		alarm_update_time (alarm);
	}
	
	g_object_set (alarm, "active", !(alarm->active), NULL);
	
	/*
	 * gtk_tree_model_get () will increase the reference count 
	 * of the alarms each time it's called. We dereference it
	 * here so they can be properly freed later with g_object_unref()
	 */
	g_object_unref (alarm);
}

static void 
alarm_object_changed (GObject *object, 
					  GParamSpec *param,
					  gpointer data)
{
	Alarm *a = ALARM (object);
	//GtkListStore *model = GTK_LIST_STORE (data);
	
	g_print ("Alarm #%d changed: %s\n", a->id, g_param_spec_get_name (param));
	
	/*g_object_notify (model, g_param_spec_get_name (param));*/
}

typedef struct _AlarmModelChangedArg {
	GtkListStore *store;
	GtkTreeIter iter;
} AlarmModelChangedArg;

/*
 * Callback for when an alarm changes
 * Emits model-row-changed
 */
static void
alarm_changed (GObject *object, 
			   GParamSpec *param,
			   gpointer data)
{
	AlarmModelChangedArg *arg = (AlarmModelChangedArg *)data;
	GtkTreePath *path;
	
	g_debug ("alarm_changed: %s", param->name);
	//if (param->name)
	
	path = gtk_tree_model_get_path (GTK_TREE_MODEL (arg->store), &(arg->iter));
	
	gtk_tree_model_row_changed (GTK_TREE_MODEL (arg->store), path, &(arg->iter));
	
	gtk_tree_path_free (path);
}

/*
 * Update list of alarms (VIEW)
 */
void
list_alarms_update (AlarmApplet *applet)
{
	GtkListStore *store = applet->list_alarms_store;
	GtkTreeIter iter;
	GList *l = NULL;
	Alarm *a;
	AlarmModelChangedArg *arg;
	
	g_debug ("list_alarms_update ()");
	
	g_assert (store);
	
	/*
	 * Clear model
	 */
	gtk_list_store_clear (store);
	
	/* 
	 * Insert alarms
	 */
	for (l = applet->alarms; l; l = l->next) {
		a = ALARM (l->data);

		g_signal_connect (a, "notify", 
				G_CALLBACK (alarm_object_changed),
				store);
		
		gtk_list_store_append (store, &iter);
		
		gtk_list_store_set (store, &iter, 0, a, -1);
		
		// TODO: arg is never freed
		arg = g_new (AlarmModelChangedArg, 1);
		arg->store = store;
		arg->iter = iter;
		
		g_signal_connect (a, "notify", G_CALLBACK (alarm_changed), arg);
		
		g_print ("Alarm #%d: %s [ref=%d]\n", a->id, a->message, G_OBJECT (a)->ref_count);
	}
}

static void 
alarm_update_renderer (GtkTreeViewColumn *tree_column, 
					   GtkCellRenderer *renderer,
					   GtkTreeModel *model,
					   GtkTreeIter *iter, 
					   gpointer data)
{
	AlarmColumn col = (AlarmColumn)data;
	Alarm *a;
	time_t time;
	struct tm *tm;
	gchar *tmp;
	
	gtk_tree_model_get (model, iter, 0, &a, -1);
	
	switch (col) {
	case TYPE_COLUMN:
		if (a->type == ALARM_TYPE_TIMER) {
			g_object_set (renderer, "icon-name", TIMER_ICON, NULL);
		} else {
			g_object_set (renderer, "icon-name", ALARM_ICON, NULL);
		}
		break;
	case ACTIVE_COLUMN:
		g_object_set (renderer, "active", a->active, NULL);
		break;
	case TIME_COLUMN:
		if (a->type == ALARM_TYPE_TIMER) {
			tm = gmtime (&(a->timer));
		} else {
			tm = localtime (&(a->time));
		}
		
		tmp = g_strdup_printf(_("%02d:%02d:%02d"), tm->tm_hour, tm->tm_min, tm->tm_sec);
		
		g_object_set (renderer, "text", tmp, NULL);
		
		g_free (tmp);
		break;
	case LABEL_COLUMN:
		g_object_set (renderer, "text", a->message, NULL);
		break;
	default:
		break;
	}
	
	/*
	 * gtk_tree_model_get () will increase the reference count 
	 * of the alarms each time it's called. We dereference it
	 * here so they can be properly freed later with g_object_unref()
	 */
	g_object_unref (a);
	
	//g_debug ("alarm_update_render: alarm #%d (%p), col %d, ref=%d", a->id, a, col, G_OBJECT (a)->ref_count);
}

static void
list_alarms_dialog_response_cb (GtkDialog *dialog,
								gint rid,
								AlarmApplet *applet)
{
	GList *l;
	Alarm *a;
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	applet->list_alarms_dialog = NULL;
	
	g_object_unref (applet->list_alarms_store);
	applet->list_alarms_store = NULL;
	
/*	g_object_unref (applet->list_alarms_view);*/
	applet->list_alarms_view = NULL;
	
	// Disconnect nofity handlers
	for (l = applet->alarms; l != NULL; l = l->next) {
		a = ALARM (l->data);
		g_signal_handlers_disconnect_matched (a, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, alarm_changed, NULL);
	}
}

static void
add_button_cb (GtkButton *button, gpointer data)
{
	Alarm *alarm;
	AlarmApplet *applet = (AlarmApplet *)data;
	
	/*
	 * Create new alarm, will fall back to defaults.
	 */
	alarm = alarm_new (applet->gconf_dir, -1);

	// Add alarm to list of alarms
	alarm_applet_alarms_add (applet, alarm);
	
	// Show edit alarm dialog
	display_edit_alarm_dialog (applet, alarm);
	
	/*
	 * Update alarms list view
	 */
	list_alarms_update (applet);
}

static Alarm *
get_selected_alarm (AlarmApplet *applet)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeModel *model;
	Alarm *a;
	
	g_assert (applet);
	g_assert (applet->list_alarms_view);

	/* Fetch selection */
	selection = gtk_tree_view_get_selection (applet->list_alarms_view);

	if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
		/* No alarms selected */
		g_debug ("get_selected_alarm: No alarms selected");
		return NULL;
	}

	gtk_tree_model_get (model, &iter, 0, &a, -1);
	
	/*
	 * gtk_tree_model_get () will increase the reference count 
	 * of the alarms each time it's called. We dereference it
	 * here so they can be properly freed later with g_object_unref()
	 */
	g_object_unref (a);
	
	return a;
}

static void
edit_button_cb (GtkButton *button, gpointer data)
{
	AlarmApplet *applet = (AlarmApplet *)data;
	Alarm *a = get_selected_alarm (applet);
	
	if (!a) {
		/* No alarms selected */
		return;
	}
	
	// Clear any running alarms
	alarm_clear (a);
	
	display_edit_alarm_dialog (applet, a);
}

static void
delete_button_cb (GtkButton *button, gpointer data)
{
	AlarmApplet *applet = (AlarmApplet *)data;
	Alarm *a = get_selected_alarm (applet);
	
	GtkWidget *dialog;
	gint response;
	
	if (!a) {
		/* No alarms selected */
		return;
	}
	
	// Clear any running alarms
	alarm_clear (a);

	dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (applet->list_alarms_dialog),
												 GTK_DIALOG_DESTROY_WITH_PARENT,
												 GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
												 _("<big><b>Delete alarm '%s'?</b></big>"),
												 a->message);
	g_object_set (dialog, 
				  "title", _("Delete Alarm?"),
				  "icon-name", ALARM_ICON,
				  NULL);
	
	gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog),
												_("Are you sure you want to delete\nalarm <b>#%d</b> labeled '<b>%s</b>'?"),
												a->id, a->message);
	
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);
	gtk_dialog_add_buttons(GTK_DIALOG (dialog), 
						   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						   GTK_STOCK_DELETE, GTK_RESPONSE_ACCEPT,
						   NULL);
	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	
	if (response == GTK_RESPONSE_ACCEPT) {
		g_debug ("delete_button_cb: GTK_RESPONSE_ACCEPT");
		
		/*
		 * Delete GCONF
		 */
		alarm_delete (a);
		
		/*
		 * Remove from applet list
		 */
		alarm_applet_alarms_remove (applet, a);
		
		/*
		 * Fill store with alarms
		 */
		list_alarms_update (applet);
	} else {
		g_debug ("delete_button_cb: GTK_RESPONSE_CANCEL");
	}
}

static void
list_alarm_selected_cb (GtkTreeView       *view,
						GtkTreePath       *path,
						GtkTreeViewColumn *column,
						gpointer           data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	Alarm *a;
	AlarmApplet *applet = (AlarmApplet *)data;
	
	model = gtk_tree_view_get_model (view);
	
	if (!gtk_tree_model_get_iter (model, &iter, path)) {
		g_warning ("Could not get iter.");
		return;
	}
	
	gtk_tree_model_get (model, &iter, 0, &a, -1);
	
	/*
	 * gtk_tree_model_get () will increase the reference count 
	 * of the alarms each time it's called. We dereference it
	 * here so they can be properly freed later with g_object_unref()
	 */
	g_object_unref (a);
	
	// Clear any running alarms
	alarm_clear (a);
	
	display_edit_alarm_dialog (applet, a);
}



void
display_list_alarms_dialog (AlarmApplet *applet)
{
	if (applet->list_alarms_dialog != NULL) {
		// Dialog already open.
		gtk_window_present (GTK_WINDOW (applet->list_alarms_dialog));
		return;
	}
	
	gchar *tmp;
	GList *l;		/* List of alarms */
	Alarm *a;
	
	GtkListStore *store;
	GtkTreeView *view;
	GtkTreeIter iter;
	GdkPixbuf *type_alarm_icon, *type_timer_icon;
	GtkCellRenderer *type_renderer, *active_renderer,
					*time_renderer, *label_renderer;
	
	GtkTreeViewColumn *type_col, *active_col,
					  *time_col, *label_col;
	
	GladeXML *ui;
	GtkButton *add_button, *edit_button, *delete_button;
	
	/* 
	 * Fetch widgets
	 */
	ui = glade_xml_new (ALARM_UI_XML, "list-alarms", NULL);
	
	applet->list_alarms_dialog = GTK_DIALOG (glade_xml_get_widget (ui, "list-alarms"));
	view = GTK_TREE_VIEW (glade_xml_get_widget (ui, "list-alarms-view"));
	applet->list_alarms_view = view;
	
	g_signal_connect (applet->list_alarms_dialog, "response", 
					  G_CALLBACK (list_alarms_dialog_response_cb), applet);
	
	// Buttons
	add_button = GTK_BUTTON (glade_xml_get_widget (ui, "add-button"));
	edit_button = GTK_BUTTON (glade_xml_get_widget (ui, "edit-button"));
	delete_button = GTK_BUTTON (glade_xml_get_widget (ui, "delete-button"));
	
	g_signal_connect (add_button, "clicked",
					  G_CALLBACK (add_button_cb), applet);
	g_signal_connect (edit_button, "clicked",
					  G_CALLBACK (edit_button_cb), applet);
	g_signal_connect (delete_button, "clicked",
					  G_CALLBACK (delete_button_cb), applet);
	
	/* 
	 * Create list store model
	 */
	store = gtk_list_store_new (1, TYPE_ALARM);
	applet->list_alarms_store = store;
	
	/*
	 * Update list of alarms
	 */
	//alarm_applet_alarms_load (applet);
	
	/*
	 * Fill store with alarms
	 */
	list_alarms_update (applet);
	
	/* 
	 * Create view cell renderers
	 */
	type_renderer = gtk_cell_renderer_pixbuf_new ();
	
	active_renderer = gtk_cell_renderer_toggle_new ();
	g_object_set (active_renderer,
				  /*"mode",  GTK_CELL_RENDERER_MODE_EDITABLE,*/
				  "activatable", TRUE,
				  NULL);
	g_signal_connect (active_renderer, "toggled", G_CALLBACK (list_alarms_toggled_cb), store);
	
	time_renderer = gtk_cell_renderer_text_new ();
	
	label_renderer = gtk_cell_renderer_text_new ();
	g_object_set (label_renderer, 
				  "weight", 1000,
				  "mode",  GTK_CELL_RENDERER_MODE_EDITABLE,
				  NULL);
	
	
	/*
	 * Create view columns
	 */
	type_col = gtk_tree_view_column_new_with_attributes(
					_("Type"), type_renderer, 
					NULL);
	
	gtk_tree_view_column_set_cell_data_func (type_col, type_renderer,
											 alarm_update_renderer, 
											 TYPE_COLUMN, NULL);
	
	
	active_col = gtk_tree_view_column_new_with_attributes(
					_("Active"), active_renderer,
					NULL);
	
	gtk_tree_view_column_set_cell_data_func (active_col, active_renderer,
											 alarm_update_renderer, 
											 ACTIVE_COLUMN, NULL);
	
	
	time_col = gtk_tree_view_column_new_with_attributes(
					_("Time"), time_renderer,
					NULL);
	
	gtk_tree_view_column_set_cell_data_func(time_col, time_renderer,
											alarm_update_renderer,
											TIME_COLUMN, NULL);
	
	
	label_col = gtk_tree_view_column_new_with_attributes(
					_("Label"), label_renderer,
					NULL);
	
	gtk_tree_view_column_set_cell_data_func(label_col, label_renderer,
											alarm_update_renderer,
											LABEL_COLUMN, NULL);
	
	/* 
	 * Set up tree view 
	 */
	gtk_tree_view_set_model (view, GTK_TREE_MODEL (store));
	
	gtk_tree_view_append_column (view, type_col);
	gtk_tree_view_append_column (view, active_col);
	gtk_tree_view_append_column (view, time_col);
	gtk_tree_view_append_column (view, label_col);
	
	/*
	 * Set up signal handlers
	 */
	g_signal_connect (view, "row-activated", G_CALLBACK (list_alarm_selected_cb), applet);
	
	
	//gtk_container_add (GTK_CONTAINER (list_alarms_box), GTK_WIDGET (view));
	//gtk_box_pack_start_defaults (GTK_BOX (list_alarms_scrol), GTK_WIDGET (view));
	
	gtk_widget_show_all (GTK_WIDGET (applet->list_alarms_dialog));
}
