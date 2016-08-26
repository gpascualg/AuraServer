include(CMakeParseArguments)

function(AddDependency)
    cmake_parse_arguments(
        ARG
        ""
        "TARGET;DEPENDENCY"
        ""
        ${ARGN}
    )

    set(${ARG_TARGET}_INCLUDE_DIRECTORIES ${${ARG_TARGET}_INCLUDE_DIRECTORIES} ${${ARG_DEPENDENCY}_INCLUDE_DIRECTORIES} CACHE INTERNAL "")
    set(${ARG_TARGET}_DEPENDENCIES ${${ARG_TARGET}_DEPENDENCIES} ${ARG_DEPENDENCY} CACHE INTERNAL "")
endfunction()

function(AddToSources)
    cmake_parse_arguments(
        ARG
        ""
        "TARGET;SRC_PATH;INC_PATH;"
        "GLOB_SEARCH"
        ${ARGN}
    )

    # Add each file and extension
    foreach (ext ${ARG_GLOB_SEARCH})
        file(GLOB TMP_SOURCES ${ARG_SRC_PATH}/*${ext})

        if(ARG_INC_PATH)
            file(GLOB TMP_INCLUDES ${ARG_INC_PATH}/*${ext})
        else()
            set(TMP_INCLUDES "")
        endif()

        set(${ARG_TARGET}_SOURCES ${${ARG_TARGET}_SOURCES} ${TMP_SOURCES} ${TMP_INCLUDES} CACHE INTERNAL "")
    endforeach()

    # Add include dirs
    if(NOT ARG_INC_PATH)
        set(ARG_INC_PATH ${ARG_SRC_PATH})
    endif()
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
        add_library(${ARG_TARGET} STATIC ${${ARG_TARGET}_SOURCES})
    endif()

    foreach (dir ${${ARG_TARGET}_INCLUDE_DIRECTORIES})
        target_include_directories(${ARG_TARGET}
            PUBLIC ${dir}
        )
    endforeach()

    foreach (dep ${${ARG_TARGET}_DEPENDENCIES})
        target_link_libraries(${ARG_TARGET}
            PUBLIC ${dep}
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
