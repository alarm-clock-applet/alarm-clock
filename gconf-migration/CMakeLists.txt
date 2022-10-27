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

target_link_directories(alarm-clock-applet-gconf-migration PRIVATE
    ${GCONF2_LIBRARY_DIRS}
    ${GIO_LIBRARY_DIRS}
)

install(
    TARGETS alarm-clock-applet-gconf-migration
    DESTINATION "bin"
)
