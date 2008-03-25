#ifndef UI_H_
#define UI_H_

#include "alarm-applet.h"

#ifdef HAVE_LIBNOTIFY
#include <libnotify/notify.h>
#endif

#define ORIENT_IS_HORIZONTAL(o)		((o) == PANEL_APPLET_ORIENT_UP || (o) == PANEL_APPLET_ORIENT_DOWN)
#define IS_HORIZONTAL(papplet)		(ORIENT_IS_HORIZONTAL (panel_applet_get_orient (papplet)))


void
display_error_dialog (const gchar *message, const gchar *secondary_text, GtkWindow *parent);

void
alarm_applet_label_update (AlarmApplet *applet);

void
alarm_applet_update_tooltip (AlarmApplet *applet);

/*
 * Shamelessly stolen from gnome-da-capplet.c
 */
void
fill_combo_box (GtkComboBox *combo_box, GList *list, const gchar *custom_label);

gboolean
alarm_applet_notification_display (AlarmApplet *applet, Alarm *alarm);

gboolean
alarm_applet_notification_close (AlarmApplet *applet);

void
alarm_applet_ui_init (AlarmApplet *applet);

void
alarm_applet_menu_init (AlarmApplet *applet);

void
media_player_error_cb (MediaPlayer *player, GError *err, GtkWindow *parent);

#endif /*UI_H_*/
