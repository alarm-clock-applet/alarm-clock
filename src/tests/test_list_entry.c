#include <glib.h>

#include "list-entry.h"

static void
entry_dump (AlarmListEntry *entry)
{
	g_print ("ListEntry %p: ");
	if (entry != NULL)
		g_print ("name: '%s', data: '%s', icon: '%s'", entry->name, entry->data, entry->icon);
	g_print ("\n");
}

int main (void)
{
	AlarmListEntry *entry = NULL;
	GList *list = NULL, *l;
	GnomeVFSResult result;
	gchar *mime;
	guint i;
	
	// Test alarm list entry alloc
	entry = alarm_list_entry_new ("Name", "Some data", "Icon");
	entry_dump (entry);
	alarm_list_entry_free(entry);
	
	entry = alarm_list_entry_new ("Name", NULL, NULL);
	entry_dump (entry);
	alarm_list_entry_free(entry);
	
	// Test alarm list entry from file
	entry = alarm_list_entry_new_file("file:///usr/share/sounds/login.wav", &result, &mime);
	entry_dump (entry);
	g_print ("VFSResult: %s, MIME: %s\n", gnome_vfs_result_to_string(result), mime);
	alarm_list_entry_free (entry);
	g_free (mime);
	
	// Test alarm list
	list = alarm_list_entry_list_new ("file:///usr/share/sounds/", NULL);
	
	g_print ("\nGot %d entries: \n---\n", g_list_length(list));
	for (l = list, i = 0; l; l = l->next, i++) {
		entry = (AlarmListEntry *)l->data;
		g_print ("#%2d: ", i);
		entry_dump (entry);
	}
	
	alarm_list_entry_list_free (&list);
	
	g_print ("After list_free: %p\n", list);
	
	return 0;
}
