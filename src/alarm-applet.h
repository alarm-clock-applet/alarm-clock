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

void alarm_applet_label_update (AlarmApplet *applet);
void alarm_applet_clear_alarms (AlarmApplet *applet);

#include "alarm.h"
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
#define ALARM_DEF_LABEL	 _("No alarms")

#ifndef VERSION
#define VERSION "0.1"
#endif

typedef enum {
	LABEL_TYPE_INVALID = 0,
	LABEL_TYPE_ACTIVE,
	LABEL_TYPE_TOTAL,
} LabelType;

struct _AlarmApplet {
	PanelApplet *parent;
	PanelAppletOrient orient;
	gchar *gconf_dir;
	
	/* Panel UI */
	GtkWidget *icon;	/* alarm icon */
	GtkWidget *label;	/* clock label */
	GtkWidget *box;		/* packing box */
	
	/* Alarms */
	GList	*alarms;
	
	/* Sounds & apps list */
	GList *sounds;
	GList *apps;
	
	/* List-alarms UI */
	GtkDialog *list_alarms_dialog;
	GtkListStore *list_alarms_store;
	GtkTreeView *list_alarms_view;
	GList *list_alarms_args;
	
	/* Open edit alarm dialogs
	 * Hashed by ID */
	GHashTable *edit_alarm_dialogs;
	
	/* Preferences */
	GtkDialog *preferences_dialog;
	
	/* Notification */
#ifdef HAVE_LIBNOTIFY
	NotifyNotification *notify;
#endif
	
	/* Label */
	GtkWidget *pref_label_show;
	GtkWidget *pref_label_type_box;
	GtkWidget *pref_label_type_active;
	GtkWidget *pref_label_type_total;
	
	/* Actual preferences data */
	gboolean show_label;
	LabelType label_type;
	
	/* GConf */
	guint listeners [N_GCONF_PREFS];
};

static void set_alarm_time (AlarmApplet *applet, guint hour, guint minute, guint second);
//static void time_changed_cb (GtkSpinButton *spinbutton, gpointer data);

void alarm_applet_sounds_load (AlarmApplet *applet);

void alarm_applet_apps_load (AlarmApplet *applet);

void alarm_applet_alarms_load (AlarmApplet *applet);

void alarm_applet_alarms_add (AlarmApplet *applet, Alarm *alarm);

void alarm_applet_alarms_remove (AlarmApplet *applet, Alarm *alarm);

void alarm_applet_destroy (AlarmApplet *applet);

G_END_DECLS

#endif /*ALARMAPPLET_H_*/
