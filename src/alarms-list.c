#include "alarms-list.h"

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
	
	//g_debug ("alarm_update_render: alarm #%d, col %d", a->id, col);
	
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

static void
list_alarms_dialog_response_cb (GtkDialog *dialog,
								gint rid,
								AlarmApplet *applet)
{
	gtk_widget_destroy (GTK_WIDGET (dialog));
	applet->list_alarms_dialog = NULL;
	
	g_object_unref (applet->list_alarms_store);
	applet->list_alarms_store = NULL;
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
	
	GtkCellRenderer *type_renderer, *active_renderer,
					*time_renderer, *label_renderer;
	
	GtkTreeViewColumn *type_col, *active_col,
					  *time_col, *label_col;
	
	GtkTreeIter iter;
	
	GdkPixbuf *type_alarm_icon, *type_timer_icon;
	
	GladeXML *ui;
	
	/* 
	 * Fetch widgets
	 */
	ui = glade_xml_new (ALARM_UI_XML, "list-alarms", NULL);
	
	applet->list_alarms_dialog = GTK_DIALOG (glade_xml_get_widget (ui, "list-alarms"));
	view = glade_xml_get_widget (ui, "list-alarms-view");
	
	g_signal_connect (applet->list_alarms_dialog, "response", 
					  G_CALLBACK (list_alarms_dialog_response_cb), applet);
	
	/* 
	 * Create list store model
	 */
	store = gtk_list_store_new (1, TYPE_ALARM);
	applet->list_alarms_store = store;
	
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
							/*TYPE_COLUMN, a->type,
							ACTIVE_COLUMN, a->active,
							TIME_COLUMN, a->time,
							LABEL_COLUMN, a->message,
							-1);*/
		
		g_print ("Alarm #%d: %s\n", a->id, a->message);
	}
	
	
	
	
	/* 
	 * Create view cell renderers
	 */
	type_renderer = gtk_cell_renderer_pixbuf_new ();
	
	active_renderer = gtk_cell_renderer_toggle_new ();
	g_object_set (active_renderer,
				  /*"mode",  GTK_CELL_RENDERER_MODE_EDITABLE,*/
				  "activatable", TRUE,
				  NULL);
	g_signal_connect (active_renderer, "toggled", list_alarms_toggled_cb, store);
	
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
	gtk_tree_view_set_model (view, store);
	
	gtk_tree_view_append_column (view, type_col);
	gtk_tree_view_append_column (view, active_col);
	gtk_tree_view_append_column (view, time_col);
	gtk_tree_view_append_column (view, label_col);
	
	
	//gtk_container_add (GTK_CONTAINER (list_alarms_box), GTK_WIDGET (view));
	//gtk_box_pack_start_defaults (GTK_BOX (list_alarms_scrol), GTK_WIDGET (view));
	
	gtk_widget_show_all (GTK_WIDGET (applet->list_alarms_dialog));
}
