cmake_minimum_required(VERSION 3.16)
if(NOT WIN32)
    message(FATAL_ERROR "Run packaging with native Windows CMake")
endif()
foreach(required APP DEST RUNTIME_DIR)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "Missing ${required}")
    endif()
endforeach()
file(MAKE_DIRECTORY "${DEST}")
file(COPY "${APP}" DESTINATION "${DEST}")
get_filename_component(APP_DIR "${APP}" DIRECTORY)
file(COPY "${APP_DIR}/images" DESTINATION "${DEST}")
file(GET_RUNTIME_DEPENDENCIES
    EXECUTABLES "${APP}"
    DIRECTORIES "${RUNTIME_DIR}"
    RESOLVED_DEPENDENCIES_VAR dependencies
    UNRESOLVED_DEPENDENCIES_VAR missing
    PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*"
    POST_EXCLUDE_REGEXES [=[.*[/\\][Ss][Yy][Ss][Tt][Ee][Mm]32[/\\].*]=])
if(missing)
    message(FATAL_ERROR "Missing runtime libraries: ${missing}")
endif()
foreach(library IN LISTS dependencies)
    file(COPY "${library}" DESTINATION "${DEST}")
endforeach()
