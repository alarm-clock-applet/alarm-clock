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