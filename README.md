# Alarm Clock
This package contains an Alarm Clock for use with an AppIndicator implementation.

![Alarm Applet Main Window](https://alarm-clock-applet.github.io/assets/screenshots/list-alarms.png)

## Installation
Installation instructions can be found on the [project website](https://alarm-clock-applet.github.io/#install).

## Requirements
This program requires the following packages:
```
cmake >= 3.10
gettext >= 0.19.8
glib-2.0 >= 2.56.4
gtk-3.0 >= 3.22.30
gio-2.0 >= 2.56.4
libnotify >= 0.7.7
gstreamer-1.0 >= 1.14.5
ayatana-appindicator3 >= 0.5.3
gnome-icon-theme
pod2man
gzip
```

This software has been tested with the specified version of each dependency as written above. It might function with older versions of these packages, however there is no support for them.

**NOTE: pod2man and gzip are optional and only needed during build time to generate the manpage**

The dependency to GConf can be removed by passing `-DENABLE_GCONF_MIGRATION=OFF` to cmake.

**WARNING: Doing so disables migration of old alarms.**

<!-- requirements_ubuntu -->
### Debian/Ubuntu-specific dependency packages
All the dependencies on a Debian/Ubuntu system can be installed with:
```
sudo apt install build-essential cmake libxml2-dev libgtk-3-dev libgstreamer1.0-dev libnotify-dev libayatana-appindicator3-dev gettext gnome-icon-theme perl gzip
```
<!-- end requirements_ubuntu -->

## Building from source
<!-- build_from_source -->
Download and extract the source code with:
```
wget --content-disposition https://github.com/alarm-clock-applet/alarm-clock/archive/refs/tags/<VERSION>.tar.gz
tar zxvf alarm-clock-<VERSION>.tar.gz
cd alarm-clock-<VERSION>
```

And compile - install with the usual:
```
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
sudo make install
```
<!-- end build_from_source -->

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
