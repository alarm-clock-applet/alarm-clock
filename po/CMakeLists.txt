set(POTFILE "alarm-clock-applet.pot")

# Dummy target for LINGUAS to force regeneration
add_custom_target(linguas DEPENDS "LINGUAS")

file(STRINGS "LINGUAS" LINGUAS)
list(FILTER LINGUAS EXCLUDE REGEX "^#")

# FIXME: Remove when Ubuntu 18.04 goes EOL
if(CMAKE_VERSION VERSION_LESS "3.12")
    set(POFILES "")
    foreach(PO IN LISTS LINGUAS)
        list(APPEND POFILES "${PO}.po")
    endforeach()
    set(LINGUAS "${POFILES}")
else()
    list(TRANSFORM LINGUAS APPEND ".po")
endif()

gettext_create_translations("${POTFILE}" ALL "${LINGUAS}")
# Add dep to LINGUAS
add_dependencies(translations linguas)

find_program(GETTEXT_XGETTEXT_EXECUTABLE xgettext)
if(NOT GETTEXT_XGETTEXT_EXECUTABLE)
    message(FATAL_ERROR "xgettext not found")
endif()

add_custom_target("pot"
    COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE}
        --from-code=UTF-8
        --keyword=_
        --add-comments
        -f po/POTFILES.in
        -o po/${POTFILE}
    DEPENDS "POTFILES.in"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
