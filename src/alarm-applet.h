#ifndef ALARMAPPLET_H_
#define ALARMAPPLET_H_

G_BEGIN_DECLS

#define ALARM_ICON 		 "stock_alarm.png"
#define ALARM_SCHEMA_DIR "/schemas/apps/alarm_applet/prefs"
#define ALARM_UI_XML	 "../ui/alarm-applet.glade"
#define ALARM_SOUNDS_DIR "/shared/workspace/gnome-alarmclock-applet/sounds"

/* GConf keys */
#define KEY_ALARMTIME		"alarm_time"
#define KEY_MESSAGE			"message"
#define KEY_STARTED			"started"
#define KEY_SHOW_LABEL		"show_label"
#define KEY_LABEL_TYPE		"label_type"
#define KEY_NOTIFY_TYPE		"notify_type"
#define KEY_SOUND_FILE		"sound_file"
#define KEY_SOUND_LOOP		"sound_repeat"
#define KEY_COMMAND			"command"
#define KEY_NOTIFY_BUBBLE	"notify_bubble"

#define N_GCONF_PREFS	10

typedef enum {
	LABEL_TYPE_INVALID = 0,
	LABEL_TYPE_ALARM,
	LABEL_TYPE_REMAIN,
} LabelType;

typedef enum {
	NOTIFY_INVALID = 0,
	NOTIFY_SOUND,
	NOTIFY_COMMAND,
} NotifyType;

enum
{
    PIXBUF_COL,
    TEXT_COL,
    N_COLUMNS
};

typedef struct {
	gchar *name;
	gchar *uri;
	gchar *icon;
} AlarmFileEntry;

typedef struct {
	PanelApplet *parent;
	GtkWidget *icon;	/* alarm icon */
	GtkWidget *label;	/* clock label */
	
	/* Alarm */
	time_t 	 alarm_time;
	gchar	*alarm_message;
	gboolean started;
	gboolean alarm_triggered;
	guint	 timer_id;
	
	/* Set-alarm */
	GtkDialog *set_alarm_dialog;
	GtkWidget *hour;
	GtkWidget *minute;
	GtkWidget *second;
	GtkWidget *message;
	
	/* Preferences */
	GtkDialog *preferences_dialog;
	
	/* Label */
	GtkWidget *pref_label_show;
	GtkWidget *pref_label_type_box;
	GtkWidget *pref_label_type_alarm;
	GtkWidget *pref_label_type_remaining;
	
	/* Notification */
	GtkWidget *pref_notify_sound;
	GtkWidget *pref_notify_sound_box;
	GtkWidget *pref_notify_sound_stock;
	GtkWidget *pref_notify_sound_combo;
	GtkWidget *pref_notify_sound_loop;
	GtkWidget *pref_notify_sound_preview;
	
	GtkWidget *pref_notify_app;
	GtkWidget *pref_notify_app_box;
	GtkWidget *pref_notify_app_select;
	GtkWidget *pref_notify_app_command_box;
	GtkWidget *pref_notify_app_command_entry;
	
	GtkWidget *pref_notify_bubble;
	
	/* Actual preferences data */
	gboolean show_label;
	LabelType label_type;
	NotifyType notify_type;
	gboolean notify_sound_loop;
	gchar *notify_command;
	gboolean notify_bubble;
	GList *sounds;
	guint stock_sounds;	// Number of stock sounds
	guint sound_pos;	// Position of the current selected sound in the sounds list.
	
	/* GStreamer */
	GstElement *player;
	GstElement *preview_player;
	guint gst_watch_id;
	
	/* GConf */
	guint listeners [N_GCONF_PREFS];
} AlarmApplet;

static void set_alarm_time (AlarmApplet *applet, guint hour, guint minute, guint second);
//static void time_changed_cb (GtkSpinButton *spinbutton, gpointer data);
static void update_label (AlarmApplet *applet);
static void timer_remove (AlarmApplet *applet);
static void set_alarm_dialog_populate (AlarmApplet *applet);
static void display_set_alarm_dialog (AlarmApplet *applet);
static void pref_label_show_changed_cb (GtkToggleButton *togglebutton, AlarmApplet *applet);
static void pref_label_type_changed_cb (GtkToggleButton *togglebutton, AlarmApplet *applet);
static void pref_notify_type_changed_cb (GtkToggleButton *togglebutton, AlarmApplet *applet);
static void pref_notify_app_changed_cb (GtkEditable *editable, AlarmApplet *applet);
static void pref_notify_app_command_changed_cb (GtkEditable *editable, AlarmApplet *applet);
static void pref_notify_bubble_changed_cb (GtkToggleButton *togglebutton, AlarmApplet *applet);
static void pref_notify_sound_loop_changed_cb (GtkToggleButton *togglebutton, AlarmApplet *applet);
static void pref_notify_sound_combo_changed_cb (GtkComboBox *combo, AlarmApplet *applet);
static void pref_play_sound_cb (GtkButton *button, AlarmApplet *applet);
static void display_error_dialog (const gchar *message, const gchar *secondary_text, GtkWindow *parent);
static const gchar *get_sound_file (AlarmApplet *applet);

G_END_DECLS

#endif /*ALARMAPPLET_H_*/
