# Alarm Clock
This package contains an Alarm Clock for use with an AppIndicator implementation.

![Alarm Applet Main Window](https://alarm-clock-applet.github.io/assets/screenshots/list-alarms.png)

## Requirements
This program requires the following packages:
```
cmake >= 3.10
gettext >= 0.19.8
glib-2.0 >= 2.56.4
gtk-3.0 >= 3.22.30
gio-2.0 >= 2.56.4
libnotify >= 0.7.7
libxml2 >= 2.9.4
gstreamer-1.0 >= 1.14.5
ayatana-appindicator3 >= 0.5.3
gnome-icon-theme
gconf-2.0 >= 3.2.6
pod2man
gzip
```

This software has been tested with the specified version of each dependency as written above. It might function with older versions of these packages, however there is no support for them.

**NOTE: pod2man and gzip are optional and only needed during build time to generate the manpage**

The dependency to GConf can be removed by passing `-DENABLE_GCONF_MIGRATION=OFF` to cmake.

**WARNING: Doing so disables migration of old alarms.**

### Ubuntu-specific packages
All the dependencies on an Ubuntu system can be installed with:
```
sudo apt install build-essential cmake libgconf2-dev libxml2-dev libgtk-3-dev libgstreamer1.0-dev libnotify-dev libayatana-appindicator3-dev gettext gnome-icon-theme perl gzip
```

## Installation
Extract with:
```
tar zxvf alarm-clock-applet-<VERSION>.tar.gz
cd alarm-clock-applet-<VERSION>
```

Compile with the usual:
```
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
sudo make install
```

## Usage

### Start applet
```
alarm-clock-applet
```

### Start applet with main window hidden
```
alarm-clock-applet --hidden
```

### Stop all alarms
```
alarm-clock-applet --stop-all
```

### Snooze all alarms
```
alarm-clock-applet --snooze-all
```
