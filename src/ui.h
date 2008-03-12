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
update_label (AlarmApplet *applet);

void
update_tooltip (AlarmApplet *applet);

gboolean
is_separator (GtkTreeModel *model, GtkTreeIter *iter, gpointer sep_index);

/*
 * Shamelessly stolen from gnome-da-capplet.c
 */
void
fill_combo_box (GtkComboBox *combo_box, GList *list, const gchar *custom_label);

void
set_alarm_dialog_response_cb (GtkDialog *dialog,
							  gint rid,
							  AlarmApplet *applet);

void
set_alarm_dialog_populate (AlarmApplet *applet);

void
display_set_alarm_dialog (AlarmApplet *applet);

gboolean
display_notification (AlarmApplet *applet);

gboolean
close_notification (AlarmApplet *applet);

void
ui_setup (AlarmApplet *applet);

void
menu_setup (AlarmApplet *applet);

void
media_player_error_cb (MediaPlayer *player, GError *err, GtkWindow *parent);

#endif /*UI_H_*/
