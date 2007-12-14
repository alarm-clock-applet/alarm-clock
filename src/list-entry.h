#ifndef LISTENTRY_H_
#define LISTENTRY_H_

#include <string.h>
#include <glib.h>
#include <libgnomevfs/gnome-vfs.h>

#include "util.h"

G_BEGIN_DECLS

typedef struct {
	gchar *name;
	gchar *data;
	gchar *icon;
} AlarmListEntry;


AlarmListEntry *
alarm_list_entry_new (const gchar *name, const gchar *data, const gchar *icon);

void
alarm_list_entry_free (AlarmListEntry *e);

AlarmListEntry *
alarm_list_entry_new_file (const gchar *uri, GnomeVFSResult *ret, gchar **mime_ret);

GList *
alarm_list_entry_list_new (const gchar *dir_uri, const gchar *supported_types[]);

void
alarm_list_entry_list_free (GList **list);

G_END_DECLS

#endif /*LISTENTRY_H_*/
