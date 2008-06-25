/*
 * list-entry.c -- Simple data structure to hold name, data and icon.
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

#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomeui/libgnomeui.h>

#include "list-entry.h"
#include "util.h"

/*
 * Creates a new AlarmListEntry.
 */
AlarmListEntry *
alarm_list_entry_new (const gchar *name, const gchar *data, const gchar *icon)
{
	AlarmListEntry *entry;
	
	entry = g_new (AlarmListEntry, 1);
	
	entry->name = NULL;
	entry->data = NULL;
	entry->icon = NULL;
	
	if (name)
		entry->name = g_strdup (name);
	if (data)
		entry->data = g_strdup (data);
	if (icon)
		entry->icon = g_strdup (icon);
	
	return entry;
}

void
alarm_list_entry_free (AlarmListEntry *e)
{
	g_free (e->data);
	g_free (e->name);
	g_free (e->icon);
	g_free (e);
}

AlarmListEntry *
alarm_list_entry_new_file (const gchar *uri, GnomeVFSResult *ret, gchar **mime_ret)
{
	AlarmListEntry *entry;
	GnomeVFSResult result;
	GnomeVFSFileInfo *info;
	GtkIconTheme *theme = NULL;
	
	if (!gnome_vfs_initialized())
		gnome_vfs_init();
	
	if (gtk_init_check (NULL, NULL))
		theme = gtk_icon_theme_get_default ();
	else
		g_warning ("gtk_init_check failed, icon lookup disabled.");
		
	info = gnome_vfs_file_info_new ();
	
	result = gnome_vfs_get_file_info (uri, info, 
				GNOME_VFS_FILE_INFO_GET_MIME_TYPE | GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
	
	if (ret != NULL)
		*ret = result;
	
	if (result != GNOME_VFS_OK) {
		g_warning ("Could not open uri: %s", uri);
		return NULL;
	}
	
	entry = g_new (AlarmListEntry, 1);
	entry->data = g_strdup (uri);
	entry->name = to_basename (info->name);
	
	if (theme != NULL)
		entry->icon = gnome_icon_lookup(theme, NULL,
								 		entry->data, NULL, info,
								 		info->mime_type, 0, 0);
	
	if (mime_ret != NULL)
		*mime_ret = g_strdup (info->mime_type);
	
	gnome_vfs_file_info_unref (info);
	
	return entry;
}

GList *
alarm_list_entry_list_new (const gchar *dir_uri, const gchar *supported_types[])
{
	GnomeVFSResult result;
	GnomeVFSFileInfoOptions options;
	GnomeVFSFileInfo *info;
	
	GtkIconTheme *theme;
	
	GList *list, *l, *flist;
	AlarmListEntry *entry;
	const gchar *mime;
	gboolean valid;
	gint i;
	
	if (!gnome_vfs_initialized())
		gnome_vfs_init();
	
	if (gtk_init_check (NULL, NULL))
		theme = gtk_icon_theme_get_default ();
	else
		g_warning ("gtk_init_check failed, icon lookup disabled.");
	
	options = GNOME_VFS_FILE_INFO_GET_MIME_TYPE | GNOME_VFS_FILE_INFO_FOLLOW_LINKS;
	
	result = gnome_vfs_directory_list_load (&list, dir_uri, options);
	
	if (result != GNOME_VFS_OK) {
		g_critical ("Could not open directory: %s", dir_uri);
		return NULL;
	}
	
	g_debug ("Loading files in %s ...", dir_uri);
	
	flist = NULL;
	
	for (l = list; l; l = l->next) {
		info = l->data;
		//g_debug ("-- %s", info->name);
		if (info->type == GNOME_VFS_FILE_TYPE_REGULAR) {
			mime = gnome_vfs_file_info_get_mime_type(info);
			//g_debug (" [ regular file: MIME: %s ]", mime);
			
			valid = TRUE;
			if (supported_types != NULL) {
				valid = FALSE;
				for (i = 0; supported_types[i] != NULL; i++) {
					if (strstr (mime, supported_types[i]) != NULL) {
						// MATCH
						//g_debug (" [ MATCH ]");
						valid = TRUE;
						break;
					}
				}
			}
			
			if (valid) {
				entry = g_new (AlarmListEntry, 1);
				entry->data = g_strdup_printf ("%s/%s", dir_uri, info->name);
				entry->name = to_basename (info->name);
				
				entry->icon = NULL;
				if (theme != NULL)
					entry->icon = gnome_icon_lookup(theme, NULL,
													entry->data, NULL, info,
													mime, 0, 0);
				
				//g_debug ("Icon found: %s", entry->icon);
				flist = g_list_append (flist, entry);
			}
		}
	}
	
	gnome_vfs_file_info_list_free (list);
	
	return flist;
}

void
alarm_list_entry_list_free (GList **list)
{
	GList *l;
	AlarmListEntry *e;
	
	g_debug ("Alarm_file_entry_list_free (%p => %p)", list, *list);
	
	// Free data
	for (l = *list; l; l = l->next) {
		e = (AlarmListEntry *)l->data;
		alarm_list_entry_free (e);
	}
	
	g_list_free (*list);
	
	*list = NULL;
}
