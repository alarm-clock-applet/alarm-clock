add_custom_target(dch
    COMMAND
        dch --distribution unstable --newversion "${CMAKE_PROJECT_VERSION}" -u medium "New upstream release ${CMAKE_PROJECT_VERSION}" --check-dirname-level 0
    WORKING_DIRECTORY
        "${CMAKE_CURRENT_LIST_DIR}"
)
