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
	
	gtk_main();
	
	return 0;
}
