#include "alarms-list.h"

static void 
list_alarms_toggled_cb  (GtkCellRendererToggle *cell_renderer,
						 gchar                 *path,
						 gpointer               user_data)
{
	GtkTreeIter iter;
	GtkListStore *model = GTK_LIST_STORE (user_data);
	
	g_debug ("Activate %s", path);
	
	gtk_tree_model_get_iter_from_string (model, &iter, path);
	
	gtk_list_store_set (model, &iter,
						ACTIVE_COLUMN, FALSE,
						-1);
}

static void 
alarm_type_get_icon (GtkTreeViewColumn *tree_column, 
					 GtkCellRenderer *renderer,
					 GtkTreeModel *model,
					 GtkTreeIter *iter, 
					 gpointer data)
{
	AlarmType type;
	
	gtk_tree_model_get (model, iter, TYPE_COLUMN, &type, -1);
	
	if (type == ALARM_TYPE_TIMER) {
		g_object_set (renderer, "icon-name", TIMER_ICON, NULL);
	} else {
		g_object_set (renderer, "icon-name", ALARM_ICON, NULL);
	}
}

static void 
alarm_time_to_str (GtkTreeViewColumn *tree_column, 
				   GtkCellRenderer *renderer,
				   GtkTreeModel *model,
				   GtkTreeIter *iter, 
				   gpointer data)
{
	time_t time;
	struct tm *tm;
	gchar *tmp;
	
	gtk_tree_model_get (model, iter, TIME_COLUMN, &time, -1);
	
	tm = localtime (&time);
	tmp = g_strdup_printf(_("%02d:%02d:%02d"), tm->tm_hour, tm->tm_min, tm->tm_sec);
	
	g_object_set (renderer, "text", tmp, NULL);
	
	g_free (tmp);
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
	GtkWidget *list_alarms_box;
	
	/* 
	 * Fetch widgets
	 */
	ui = glade_xml_new (ALARM_UI_XML, "list-alarms", NULL);
	
	applet->list_alarms_dialog = GTK_DIALOG (glade_xml_get_widget (ui, "list-alarms"));
	list_alarms_box = glade_xml_get_widget (ui, "list-alarms-box");
	
	
	/* 
	 * Create list store model
	 */
	store = gtk_list_store_new (ALARMS_N_COLUMNS,
								G_TYPE_INT,
								G_TYPE_BOOLEAN,
								G_TYPE_INT,
								G_TYPE_STRING);
	
	/* 
	 * Insert alarms
	 */
	for (l = applet->alarms; l; l = l->next) {
		a = ALARM (l->data);
		
		gtk_list_store_append (store, &iter);
		
		gtk_list_store_set (store, &iter,
							TYPE_COLUMN, a->type,
							ACTIVE_COLUMN, a->active,
							TIME_COLUMN, a->time,
							LABEL_COLUMN, a->message,
							-1);
		
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
	
	gtk_tree_view_column_set_cell_data_func(type_col, type_renderer,
											alarm_type_get_icon, 
											NULL, NULL);
	
	
	active_col = gtk_tree_view_column_new_with_attributes(
					_("Active"), active_renderer,
					"active", ACTIVE_COLUMN,
					NULL);
	
	
	time_col = gtk_tree_view_column_new_with_attributes(
					_("Time"), time_renderer,
					NULL);
	
	gtk_tree_view_column_set_cell_data_func(time_col, time_renderer,
											alarm_time_to_str,
											NULL, NULL);
	
	
	label_col = gtk_tree_view_column_new_with_attributes(
					_("Label"), label_renderer,
					"text", LABEL_COLUMN,
					NULL);
	
	/* 
	 * Create tree view 
	 */
	view = g_object_new (GTK_TYPE_TREE_VIEW,
						 "model", store,
						 NULL);
	
	gtk_tree_view_append_column (view, type_col);
	gtk_tree_view_append_column (view, active_col);
	gtk_tree_view_append_column (view, time_col);
	gtk_tree_view_append_column (view, label_col);
	
	
	gtk_box_pack_start_defaults (GTK_BOX (list_alarms_box), GTK_WIDGET (view));
	
	gtk_widget_show_all (GTK_WIDGET (applet->list_alarms_dialog));
}
