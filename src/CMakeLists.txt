find_package(PkgConfig REQUIRED)
find_package(LibXml2 REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(GCONF2 REQUIRED gconf-2.0)
pkg_check_modules(GST REQUIRED gstreamer-1.0)
pkg_check_modules(LIBNOTIFY REQUIRED libnotify)
pkg_check_modules(APPINDICATOR REQUIRED ayatana-appindicator3-0.1)

add_executable(alarm-clock-applet
    alarm-applet.c alarm-applet.h
    player.c player.h
    util.c util.h
    list-entry.c list-entry.h
    alarm.c alarm.h
    alarm-gconf.c alarm-gconf.h
    ui.c ui.h
    alarm-actions.c alarm-actions.h
    alarm-list-window.c alarm-list-window.h
    alarm-settings.c alarm-settings.h
    prefs.c prefs.h
)
set_property(TARGET alarm-clock-applet PROPERTY C_STANDARD 11)

target_compile_options(alarm-clock-applet PRIVATE ${GTK3_CFLAGS_OTHER})

target_link_options(alarm-clock-applet PRIVATE "-rdynamic")

target_link_libraries(alarm-clock-applet PRIVATE
    ${GTK3_LIBRARIES}
    ${GCONF2_LIBRARIES}
    ${GST_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${LIBNOTIFY_LIBRARIES}
    ${APPINDICATOR_LIBRARIES}
)

target_include_directories(alarm-clock-applet PRIVATE
    ${GTK3_INCLUDE_DIRS}
    ${GCONF2_INCLUDE_DIRS}
    ${GST_INCLUDE_DIRS}
    ${LIBXML2_INCLUDE_DIRS}
    ${LIBNOTIFY_INCLUDE_DIRS}
    ${APPINDICATOR_INCLUDE_DIRS}
)

target_link_directories(alarm-clock-applet PRIVATE
    ${GTK3_LIBRARY_DIRS}
    ${GCONF2_LIBRARY_DIRS}
    ${GST_LIBRARY_DIRS}
    ${LIBXML2_LIBRARY_DIRS}
    ${LIBNOTIFY_LIBRARY_DIRS}
    ${APPINDICATOR_LIBRARY_DIRS}
)

configure_file(
    "${CMAKE_SOURCE_DIR}/src/config.h.in"
    "${CMAKE_BINARY_DIR}/config.h"
)

# Binary
install(
    TARGETS alarm-clock-applet
    DESTINATION "bin"
)

# ui and css
install(
    FILES
        "${CMAKE_SOURCE_DIR}/data/alarm-clock.ui"
        "${CMAKE_SOURCE_DIR}/data/alarm-clock.css"
    DESTINATION "share/alarm-clock-applet/"
)

# Desktop shortcut
install(
    FILES
        "${CMAKE_SOURCE_DIR}/data/alarm-clock-applet.desktop"
    DESTINATION "share/applications/"
)

# Gconf schema
install(
    FILES
        "${CMAKE_SOURCE_DIR}/data/alarm-clock.schemas"
    DESTINATION "etc/gconf/schemas/"
)

# Icons
install(DIRECTORY "${CMAKE_SOURCE_DIR}/data/icons/icons"
    DESTINATION "share"
    FILE_PERMISSIONS
    OWNER_READ OWNER_WRITE
    GROUP_READ
    WORLD_READ
    DIRECTORY_PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE
)

# Needed to regenerate the icon cache
# 3.12 first introduced TOUCH, and Ubuntu 18.04 has 3.10
install(
    CODE
    "file(WRITE \"${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/.xdg-icon-resource-dummy\" \"\")
    file(REMOVE \"${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/.xdg-icon-resource-dummy\")"
)

# Uninstall target
add_custom_target(uninstall
    COMMAND xargs -r -t rm -f < "${CMAKE_BINARY_DIR}/install_manifest.txt"
)
