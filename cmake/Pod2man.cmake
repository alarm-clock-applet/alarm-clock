# SPDX-License-Identifier: BSD-3-Clause
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products 
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Use pod2man to generate manual pages from .pod files

# Usage:  pod2man(<podfile> <release-string> <man-section> <heading-center-text>)
#
# E.g.: pod2man("/path/to/file/mypod.pod" "1.2.3" 1 "My Manual Pages")

include(GNUInstallDirs)

find_program(POD2MAN pod2man)
if(NOT POD2MAN)
    message(STATUS "Could not find pod2man - man pages disabled")
endif()

find_program(GZIP gzip)
if(NOT GZIP)
    message(STATUS "Could not find gzip - man pages uncompressed")
endif()

macro(pod2man PODFILE_FULL RELEASE SECTION CENTER)
    get_filename_component(PODFILE ${PODFILE_FULL} NAME)
    string(REPLACE "." ";" PODFILE_LIST ${PODFILE})
    list(GET PODFILE_LIST 0 NAME)
    list(GET PODFILE_LIST 1 LANG)
    string(TOUPPER ${NAME} NAME_UPCASE)
    if(${LANG} STREQUAL "pod")
        set(LANG "")
    endif()

    if(NOT EXISTS ${PODFILE_FULL})
        message(FATAL_ERROR "Could not find pod file ${PODFILE_FULL} to generate man page")
    endif(NOT EXISTS ${PODFILE_FULL})

    if(POD2MAN)
        if(LANG)
            set(MANPAGE_TARGET "man-${NAME}-${LANG}")
            set(MANFILE_FULL "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.${LANG}.${SECTION}")
            set(MANFILE_FULL_GZ "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.${LANG}.${SECTION}.gz")
            set(MANFILE_DEST "${CMAKE_INSTALL_FULL_MANDIR}/${LANG}/man${SECTION}")
        else()
            set(MANPAGE_TARGET "man-${NAME}")
            set(MANFILE_FULL "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.${SECTION}")
            set(MANFILE_FULL_GZ "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.${SECTION}.gz")
            set(MANFILE_DEST "${CMAKE_INSTALL_FULL_MANDIR}/man${SECTION}")
        endif()
        add_custom_command(
            OUTPUT ${MANFILE_FULL}
            COMMAND ${POD2MAN} --utf8 --section="${SECTION}" --center="${CENTER}"
                --release="${RELEASE}" --name="${NAME_UPCASE}" "${PODFILE_FULL}" "${MANFILE_FULL}"
        )

        if(GZIP AND WITH_MANPAGE_COMPRESSION)
            add_custom_command(
                OUTPUT ${MANFILE_FULL_GZ}
	        COMMAND ${GZIP} -f -k --best -n "${MANFILE_FULL}"
	        DEPENDS ${MANFILE_FULL}
            )
            add_custom_target(${MANPAGE_TARGET} ALL
                DEPENDS ${MANFILE_FULL_GZ}
            )
            install(
                FILES ${MANFILE_FULL_GZ}
                RENAME ${NAME}.${SECTION}.gz
                DESTINATION ${MANFILE_DEST}
            )
        else()
            add_custom_target(${MANPAGE_TARGET} ALL
                DEPENDS ${MANFILE_FULL}
            )
            install(
                FILES ${MANFILE_FULL}
                RENAME ${NAME}.${SECTION}
                DESTINATION ${MANFILE_DEST}
            )
        endif()
    endif()
endmacro(pod2man PODFILE NAME SECTION CENTER)
