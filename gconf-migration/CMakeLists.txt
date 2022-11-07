pkg_check_modules(GCONF2 REQUIRED gconf-2.0)
pkg_check_modules(GIO REQUIRED gio-2.0)

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
    DESTINATION "etc/gconf/schemas/"
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
