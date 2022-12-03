# SPDX-License-Identifier: GPL-2.0-or-later
cmake_minimum_required(VERSION 3.10)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

project("alarm-clock-applet" VERSION 0.4.0 LANGUAGES C)

include_directories("${CMAKE_BINARY_DIR}")
include(GNUInstallDirs)

find_package(PkgConfig REQUIRED)
find_package(Gettext REQUIRED)

# Uninstall target
add_custom_target(uninstall
    COMMAND xargs -r -t rm -f < "${CMAKE_BINARY_DIR}/install_manifest.txt"
)

option(ENABLE_GCONF_MIGRATION "Enables GConf to GSettings migration for existing alarms (and adds a dependency to GConf)." ON)
option(ALLOW_MISSING_GCONF "Allows the project to build with GConf missing. Useful for existing installations (AUR) that already had GConf." OFF)
if(ENABLE_GCONF_MIGRATION)
    add_subdirectory("gconf-migration")
endif()
option(WITH_MANPAGE_COMPRESSION "Compresses the manpage with Gzip if installed." ON)

add_compile_options("-Wshadow")

add_subdirectory("po")
add_subdirectory("src")
add_subdirectory("debian")
