include(CMakeParseArguments)

function(AddToSources)
    cmake_parse_arguments(
        ARG
        ""
        "TARGET;SRC_PATH;INC_PATH;"
        "GLOB_SEARCH"
        ${ARGN}
    )

    if(NOT ARG_INC_PATH)
        set(ARG_INC_PATH ${ARG_SRC_PATH})
    endif()

    foreach (ext ${ARG_GLOB_SEARCH})
        file(GLOB TMP_SOURCES ${ARG_SRC_PATH}/*${ext})
        if (NOT "${ARG_SRC_PATH}" STREQUAL "${ARG_INC_PATH}")
            file(GLOB TMP_INCLUDES ${ARG_INC_PATH}/*${ext})
        endif()

        set(${ARG_TARGET}_SOURCES ${${ARG_TARGET}_SOURCES} ${TMP_SOURCES} ${TMP_INCLUDES} CACHE INTERNAL "")
    endforeach()

    set(${ARG_TARGET}_INCLUDE_DIRECTORIES ${${ARG_TARGET}_INCLUDE_DIRECTORIES} ${ARG_INC_PATH} CACHE INTERNAL "")
endfunction()

function(BuildNow)
    cmake_parse_arguments(
        ARG
        "EXECUTABLE;STATIC_LIB;"
        "TARGET;BUILD_FUNC;OUTPUT_NAME;"
        ""
        ${ARGN}
    )

    if (ARG_EXECUTABLE)
        add_executable(${ARG_TARGET} ${${ARG_TARGET}_SOURCES})
    elseif (ARG_STATIC_LIB)
        add_library(STATIC ${ARG_TARGET} ${${ARG_TARGET}_SOURCES})
    endif()

    foreach (dir ${${ARG_TARGET}_INCLUDE_DIRECTORIES})
        target_include_directories(${ARG_TARGET}
            PUBLIC ${dir}
        )
    endforeach()

    if (UNIX)
        if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR "${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
            set_target_properties(${ARG_TARGET} PROPERTIES
                COMPILE_DEFINITIONS DEBUG
            )
        else()
            set_target_properties(${ARG_TARGET} PROPERTIES
                COMPILE_DEFINITIONS NDEBUG
            )
        endif()
    endif()

    set_target_properties(${ARG_TARGET} PROPERTIES
        OUTPUT_NAME ${ARG_OUTPUT_NAME}
    )
endfunction()
