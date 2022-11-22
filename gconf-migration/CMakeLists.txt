# SPDX-License-Identifier: GPL-2.0-or-later
pkg_check_modules(GIO REQUIRED gio-2.0)
pkg_check_modules(GCONF2 gconf-2.0)
if(NOT GCONF2_FOUND)
    if(ALLOW_MISSING_GCONF)
        message(WARNING "GConf was not found but the package will still be built.\nIf you want to migrate old alarms created by alarm-clock-applet < 0.4.0, please install GConf and re-run CMake/rebuild the package.")
        add_custom_target(alarm-clock-applet-gconf-migration)
        return()
    else()
        message(FATAL_ERROR "GConf not found.\nPlease install it or pass `-DENABLE_GCONF_MIGRATION=OFF` if you do not want to migrate alarms created by alarm-clock-applet < 0.4.0.")
    endif()
endif()

add_executable(alarm-clock-applet-gconf-migration alarm-clock-gconf-migration.c)
set_property(TARGET alarm-clock-applet-gconf-migration PROPERTY C_STANDARD 11)

target_link_libraries(alarm-clock-applet-gconf-migration PRIVATE
    ${GCONF2_LIBRARIES}
    ${GIO_LIBRARIES}
)

target_include_directories(alarm-clock-applet-gconf-migration PRIVATE
    ${GCONF2_INCLUDE_DIRS}
    ${GIO_INCLUDE_DIRS}
)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.13")
    target_link_directories(alarm-clock-applet-gconf-migration PRIVATE
        ${GCONF2_LIBRARY_DIRS}
        ${GIO_LIBRARY_DIRS}
    )
endif()

install(
    TARGETS alarm-clock-applet-gconf-migration
    DESTINATION "bin"
)

install(
    FILES
        "alarm-clock.schemas"
    DESTINATION "${CMAKE_INSTALL_FULL_SYSCONFDIR}/gconf/schemas/"
)

set(GCONF_CONFIG_SOURCE "")
if(EXISTS "/etc/gconf/gconf.xml.defaults")
    set(GCONF_CONFIG_SOURCE "xml:merged:/etc/gconf/gconf.xml.defaults")
endif()

find_program(GCONFTOOL_2_EXECUTABLE gconftool-2)
if(NOT GCONFTOOL_2_EXECUTABLE)
    message(FATAL_ERROR "gconftool-2 not found")
endif()

install(
    CODE
        "set(ENV{GCONF_CONFIG_SOURCE} \"https://gitlab.kitware.com/cmake/cmake/-/issues/22624\")
        set(ENV{GCONF_CONFIG_SOURCE} \"${GCONF_CONFIG_SOURCE}\")
        execute_process(COMMAND ${GCONFTOOL_2_EXECUTABLE}
            --makefile-install-rule
            alarm-clock.schemas
            WORKING_DIRECTORY \"${CMAKE_CURRENT_LIST_DIR}\"
        )"
)

add_custom_target(uninstall_gconf_schema
    COMMAND
        ${CMAKE_COMMAND} -E env GCONF_CONFIG_SOURCE="${GCONF_CONFIG_SOURCE}"
        ${GCONFTOOL_2_EXECUTABLE}
            --makefile-uninstall-rule
            alarm-clock.schemas
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
)
add_dependencies(uninstall uninstall_gconf_schema)
