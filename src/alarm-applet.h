#ifndef ALARMAPPLET_H_
#define ALARMAPPLET_H_

#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <libxml/parser.h>

#include <gtk/gtk.h>
#include <gnome.h>
#include <glade/glade.h>
#include <gdk/gdkkeysyms.h>
#include <gconf/gconf-client.h>
#include <gst/gst.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-application-registry.h>

#include <panel-applet.h>
#include <panel-applet-gconf.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

G_BEGIN_DECLS

typedef struct _AlarmApplet AlarmApplet;

gchar *supported_sound_mime_types[];
GHashTable *app_command_map;

void update_label (AlarmApplet *applet);
void timer_remove (AlarmApplet *applet);
gboolean set_sound_file (AlarmApplet *applet, const gchar *uri);
void timer_start (AlarmApplet *applet);
void clear_alarm (AlarmApplet *applet);
void set_alarm_dialog_populate (AlarmApplet *applet);
const gchar *get_sound_file (AlarmApplet *applet);
void player_preview_start (AlarmApplet *applet);

#include "prefs.h"
#include "alarm-gconf.h"
#include "player.h"
#include "util.h"
#include "list-entry.h"
#include "ui.h"

#define ALARM_NAME		 "Alarm Clock Applet"
#define ALARM_ICON 		 "alarm-clock"
#define TIMER_ICON		 "alarm-timer"
#define ALARM_SCHEMA_DIR "/schemas/apps/alarm_applet/prefs"
#define ALARM_UI_XML	 GNOME_GLADEDIR "/alarm-applet.glade"
#define ALARM_SOUNDS_DIR GNOME_SOUNDSDIR
#define ALARM_DEF_LABEL	 _("No alarm")

#ifndef VERSION
#define VERSION "0.1"
#endif

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

struct _AlarmApplet {
	PanelApplet *parent;
	PanelAppletOrient orient;
	
	/* Panel UI */
	GtkWidget *icon;	/* alarm icon */
	GtkWidget *label;	/* clock label */
	GtkWidget *box;		/* packing box */
	
	/* Alarms */
	GList	*alarms;
	
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
	
	/* List-alarms */
	GtkDialog *list_alarms_dialog;
	
	/* Notification */
#ifdef HAVE_LIBNOTIFY
	NotifyNotification *notify;
#endif
	
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
	GtkWidget *pref_notify_app_combo;
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
	GList *apps;
	
	/* MediaPlayer */
	MediaPlayer *player;
	MediaPlayer *preview_player;
	
	/* GConf */
	guint listeners [N_GCONF_PREFS];
};

static void set_alarm_time (AlarmApplet *applet, guint hour, guint minute, guint second);
//static void time_changed_cb (GtkSpinButton *spinbutton, gpointer data);

G_END_DECLS

#endif /*ALARMAPPLET_H_*/
