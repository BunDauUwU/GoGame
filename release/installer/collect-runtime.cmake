cmake_minimum_required(VERSION 3.23)
file(GLOB_RECURSE plugins "${payload}/plugins/*.dll" "${payload}/qml/*.dll")
file(GET_RUNTIME_DEPENDENCIES
    EXECUTABLES "${payload}/appGoGame.exe"
    LIBRARIES ${plugins}
    DIRECTORIES ${runtime_dirs}
    PRE_EXCLUDE_REGEXES "^api-ms-" "^ext-ms-"
    POST_EXCLUDE_REGEXES ".*/[Ww][Ii][Nn][Dd][Oo][Ww][Ss]/.*"
    RESOLVED_DEPENDENCIES_VAR resolved
    UNRESOLVED_DEPENDENCIES_VAR unresolved)
if(unresolved)
    message(FATAL_ERROR "Missing application runtime DLLs: ${unresolved}. Add their directories with --runtime-dir.")
endif()
foreach(dependency IN LISTS resolved)
    get_filename_component(directory "${dependency}" DIRECTORY)
    if(NOT "${directory}" STREQUAL "${payload}")
        file(COPY "${dependency}" DESTINATION "${payload}")
    endif()
endforeach()
