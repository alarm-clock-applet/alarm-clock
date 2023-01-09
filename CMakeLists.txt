# SPDX-License-Identifier: GPL-2.0-or-later
cmake_minimum_required(VERSION 3.10)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(HARDCODED_VERSION "0.4.1")
set(IS_RELEASE FALSE)

if(IS_RELEASE)
    set(GIT_PROJECT_VER "${HARDCODED_VERSION}")
else()
    # Ask git for the version number
    execute_process(COMMAND git describe --tags --dirty
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_PROJECT_VER
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT GIT_PROJECT_VER)
        # Try asking dpkg-parsechangelog for a version number
        execute_process(COMMAND dpkg-parsechangelog -S Version
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_PROJECT_VER
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        # Did dpkg-parsechangelog fail too?
        if(NOT GIT_PROJECT_VER)
            set(GIT_PROJECT_VER "${HARDCODED_VERSION}-unknown")
        elseif(NOT(GIT_PROJECT_VER MATCHES "-"))
            set(GIT_PROJECT_VER "${GIT_PROJECT_VER}-deb")
        endif()
    endif()
endif()

string(REGEX REPLACE "-.*" "" PARSED_PROJECT_VER "${GIT_PROJECT_VER}")
project("alarm-clock-applet" VERSION "${PARSED_PROJECT_VER}" LANGUAGES C)

# Manually set the version string to keep the suffix
set(PROJECT_VERSION "${GIT_PROJECT_VER}")

if(NOT(PARSED_PROJECT_VER VERSION_EQUAL HARDCODED_VERSION))
    message(AUTHOR_WARNING "Hardcoded version is not equal to git version. Please correct it!")
endif()

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

add_compile_options("-Wshadow"
    "-Werror=return-type"
)

add_subdirectory("po")
add_subdirectory("src")
add_subdirectory("debian")
