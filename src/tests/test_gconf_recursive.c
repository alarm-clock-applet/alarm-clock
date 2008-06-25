/*
 * test_gconf_recursive.c -- Test utility for recursively unsetting a gconf dir.
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

#include <glib.h>
#include <gconf/gconf-client.h>


static void
notify (GConfClient *client,
		guint cnxn_id,
		GConfEntry *entry,
		gpointer user_data)
{
	g_debug ("notify (%p, %d, %p, %p)", client, cnxn_id, entry, user_data);
	
	if (entry) {
		g_debug ("\t%s = %p", entry->key, entry->value);
	}
}

int main(void)
{
	GConfClient *client;
	GError *err = NULL;
	gboolean result;
	
	GMainLoop *loop = g_main_loop_new(g_main_context_default(), FALSE);
	
	client = gconf_client_get_default();
	
	gconf_client_add_dir (client, "/apps/atest", 
						  GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);
	gconf_client_notify_add (client, "/apps/atest", notify, NULL, NULL, NULL);
	
	result = gconf_client_recursive_unset (client, "/apps/atest/foo", GCONF_UNSET_INCLUDING_SCHEMA_NAMES, &err);
	
	g_debug ("result: %d", result);
	g_debug ("err: %p", err);
	
	gconf_client_suggest_sync (client, &err);
	
	g_debug ("result: %d", result);
	g_debug ("err: %p", err);
	
	g_main_loop_run (loop);
	
	//result = gconf_client_unset (client, "/apps/atest/fooz", &err);
	//result = gconf_client_
	
	/*g_debug ("result: %d", result);
	g_debug ("err: %p", err);*/
	
	return 0;
}