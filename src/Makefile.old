all:
	gcc -Wall -ggdb `pkg-config --cflags --libs libpanelapplet-2.0 libglade-2.0 gstreamer-0.10` -o alarm-applet alarm-applet.c

clean:
	rm alarm-applet
	sudo rm /usr/lib/bonobo/servers/GNOME_AlarmApplet.server

install: all
	sudo cp GNOME_AlarmApplet.server /usr/lib/bonobo/servers/
