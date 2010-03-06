/*
 * test_libnotify.c -- Test libnotify
 * 
 * Copyright (C) 2010 Johannes H. Jensen <joh@pseudoberries.com>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 * Authors:
 * 		Johannes H. Jensen <joh@pseudoberries.com>
 */

#include <libnotify/notify.h>
#include <stdio.h>

#include <glib.h>
#include <gtk/gtk.h>
/*
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>*/

GMainLoop *loop;
NotifyNotification *n;

gboolean
close_notify (gpointer data)
{
    g_debug("CLOSE NOTIFY: %d", notify_notification_close (n, NULL));

    return FALSE;
}

int
main(int argc, char *argv[])
{
	gtk_init(&argc, &argv);
    
	if (!notify_init("Alarm Clock"))
		return 1;

    GtkStatusIcon *si = gtk_status_icon_new_from_icon_name ("alarm-clock");

	/* Stock icon */
	n = notify_notification_new_with_status_icon("Alarm & <b>Test</b>",
                                                 "Click the alarm icon to snooze",
                                                 "alarm-timer", si);

	if (!notify_notification_show(n, NULL))
	{
		fprintf(stderr, "failed to send notification\n");
		return 1;
	}

    g_timeout_add_seconds (1, close_notify, NULL);

    gtk_main ();
    
	return 0;
}
