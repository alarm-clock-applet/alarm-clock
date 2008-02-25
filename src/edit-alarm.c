#include "edit-alarm.h"
#include "alarm-applet.h"
#include "alarm.h"

static void
alarm_settings_time_update (AlarmSettingsDialog *dialog);

static void
time_changed_cb (GtkSpinButton *spinbutton, gpointer data);

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

static void
type_toggle_cb (GtkToggleButton *toggle, gpointer data)
{
	AlarmSettingsDialog *dialog = (AlarmSettingsDialog *)data;
	GtkWidget *toggle2 = (GTK_WIDGET (toggle) == dialog->clock_toggle) ? dialog->timer_toggle : dialog->clock_toggle;
	gboolean toggled = gtk_toggle_button_get_active(toggle);
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle2), !toggled);
	
	if (GTK_WIDGET (toggle) == dialog->clock_toggle && toggled) {
		g_object_set (dialog->alarm, "type", ALARM_TYPE_CLOCK, NULL);
	} else {
		g_object_set (dialog->alarm, "type", ALARM_TYPE_TIMER, NULL);
	}
	
	time_changed_cb (dialog->hour_spin, dialog);
}

static void
time_changed_cb (GtkSpinButton *spinbutton, gpointer data)
{
	AlarmSettingsDialog *dialog = (AlarmSettingsDialog *)data;
	guint hour, min, sec;
	
	hour = gtk_spin_button_get_value (GTK_SPIN_BUTTON (dialog->hour_spin));
	min = gtk_spin_button_get_value (GTK_SPIN_BUTTON (dialog->min_spin));
	sec = gtk_spin_button_get_value (GTK_SPIN_BUTTON (dialog->sec_spin));
	
	if (dialog->alarm->type == ALARM_TYPE_CLOCK) {
		alarm_set_time (dialog->alarm, hour, min, sec);
	} else {
		alarm_set_timer (dialog->alarm, hour, min, sec);
	}
}

void
alarm_type_changed (GObject *object, 
					GParamSpec *param,
					gpointer data)
{
	AlarmSettingsDialog *dialog = (AlarmSettingsDialog *)data;
	Alarm *alarm = ALARM (object);
	
	if (alarm->type == ALARM_TYPE_CLOCK) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->clock_toggle), TRUE);
	} else {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->timer_toggle), TRUE);
	}
}

void
alarm_time_changed (GObject *object, 
					GParamSpec *param,
					gpointer data)
{
	AlarmSettingsDialog *dialog = (AlarmSettingsDialog *)data;
	
	/* Only interesting if alarm is a CLOCK */
	if (dialog->alarm->type != ALARM_TYPE_CLOCK)
		return;
	
	alarm_settings_time_update (dialog);
}

void
alarm_timer_changed (GObject *object, 
					 GParamSpec *param,
					 gpointer data)
{
	AlarmSettingsDialog *dialog = (AlarmSettingsDialog *)data;
	
	/* Only interesting if alarm is a TIMER */
	if (dialog->alarm->type != ALARM_TYPE_TIMER)
		return;
	
	alarm_settings_time_update (dialog);
}

static void
alarm_settings_time_update (AlarmSettingsDialog *dialog)
{
	struct tm *tm;
	
	if (dialog->alarm->type == ALARM_TYPE_CLOCK) {
		tm = localtime(&(dialog->alarm->time));
	} else {
		// TIMER
		tm = gmtime (&(dialog->alarm->timer));
	}
	
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->hour_spin), tm->tm_hour);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->min_spin), tm->tm_min);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->sec_spin), tm->tm_sec);
}

static void
alarm_settings_dialog_update (AlarmSettingsDialog *dialog)
{
	Alarm *alarm = ALARM (dialog->alarm);
	
	if (alarm->type == ALARM_TYPE_TIMER) {
		g_object_set (dialog->timer_toggle, "active", TRUE, NULL);
	} else {
		g_object_set (dialog->clock_toggle, "active", TRUE, NULL);
	}
	
	g_object_set (dialog->label_entry, "text", alarm->message, NULL);
}

static AlarmSettingsDialog *
alarm_settings_dialog_new (Alarm *alarm)
{
	AlarmSettingsDialog *dialog;
	GtkWidget *clock_content, *timer_content;
	
	dialog = g_new(AlarmSettingsDialog, 1);
	
	GladeXML *ui = glade_xml_new (ALARM_UI_XML, "edit-alarm", NULL);
	
	
	
	dialog->alarm = alarm;
	dialog->dialog = glade_xml_get_widget (ui, "edit-alarm");
	
	/*
	 * TYPE TOGGLE BUTTONS
	 */
	dialog->clock_toggle = glade_xml_get_widget (ui, "toggle-clock");
	clock_content = create_img_label ("Alarm Clock", "alarm-clock");
	
	dialog->timer_toggle = glade_xml_get_widget (ui, "toggle-timer");
	timer_content = create_img_label ("Timer", "alarm-timer");
	
	gtk_container_add (GTK_CONTAINER (dialog->clock_toggle), clock_content);
	gtk_widget_show_all (GTK_WIDGET (dialog->clock_toggle));
	
	gtk_container_add (GTK_CONTAINER (dialog->timer_toggle), timer_content);
	gtk_widget_show_all (GTK_WIDGET (dialog->timer_toggle));
	
	g_signal_connect (dialog->clock_toggle, "toggled", G_CALLBACK (type_toggle_cb), dialog);
	g_signal_connect (dialog->timer_toggle, "toggled", G_CALLBACK (type_toggle_cb), dialog);
	
	/*
	 * GENERAL SETTINGS
	 */
	dialog->label_entry = glade_xml_get_widget (ui, "label-entry");
	gtk_widget_grab_focus (dialog->label_entry);
	
	dialog->hour_spin = glade_xml_get_widget (ui, "hour-spin");
	dialog->min_spin = glade_xml_get_widget (ui, "minute-spin");
	dialog->sec_spin = glade_xml_get_widget (ui, "second-spin");
	
	g_signal_connect (dialog->hour_spin, "value-changed", G_CALLBACK (time_changed_cb), dialog);
	g_signal_connect (dialog->min_spin, "value-changed", G_CALLBACK (time_changed_cb), dialog);
	g_signal_connect (dialog->sec_spin, "value-changed", G_CALLBACK (time_changed_cb), dialog);
	
	/*
	 * Populate widgets
	 */
	alarm_settings_dialog_update (dialog);
	
	/*
	 * Bind widgets
	 */
	alarm_bind (alarm, "message", dialog->label_entry, "text");
	
	/*
	 * Special widgets require special attention!
	 */
	g_signal_connect (alarm, "notify::type", G_CALLBACK (alarm_type_changed), dialog);
	g_signal_connect (alarm, "notify::time", G_CALLBACK (alarm_time_changed), dialog);
	g_signal_connect (alarm, "notify::timer", G_CALLBACK (alarm_timer_changed), dialog);
}

void
display_add_alarm_dialog (AlarmApplet *applet)
{
	Alarm *alarm;
	AlarmSettingsDialog *dialog;
	
	/*
	 * Create new alarm, will fall back to defaults.
	 */
	alarm = alarm_new (applet->gconf_dir, -1);
	
	dialog = alarm_settings_dialog_new (alarm);
}

void
display_edit_alarm_dialog (Alarm *a)
{
	
}
