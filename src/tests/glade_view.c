/*
 * glade_view.c -- Test utility for running a glade file.
 * 
 * Copyright (C) 2007-2008 Johannes H. Jensen <joh@pseudoberries.com>
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

#include <gtk/gtk.h>
#include <glade/glade.h>

int main (int argc, char **argv)
{
	gchar *file, *root = NULL;
	GladeXML *ui;
	
	gtk_init (&argc, &argv);
	
	if (argc < 2) {
		g_printerr ("Usage: %s <glade-file> [root]\n", argv[0]);
		return 1;
	}
	
	file = argv[1];
	if (argc > 2)
		root = argv[2];
	
	ui = glade_xml_new (file, root, NULL);
	
	if (!ui) {
		g_printerr ("Could not open '%s'. Exiting.\n", file);
		return 1;
	}
	
	GtkWidget *checkbox = glade_xml_get_widget(ui, "snooze-check");
	/* you can't do this from glade */
	GtkWidget *checkbox_label = gtk_bin_get_child (GTK_BIN (checkbox));
	g_object_set (G_OBJECT (checkbox_label), "use_markup", TRUE, NULL);
	
	gtk_main();
	
	return 0;
}
