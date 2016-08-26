project(DBOSupreme)

set(SOURCE_FOLDERS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/crypto
)

foreach(folder ${SOURCE_FOLDERS})
    AddToSources(
        TARGET DBOSupreme
        SRC_PATH ${folder}
        GLOB_SEARCH ".hpp;.cpp"
    )

    AddToSources(
        TARGET DBOSupreme_LIB
        SRC_PATH ${folder}
        GLOB_SEARCH ".hpp;.cpp"
    )
endforeach()

# -[ Build executable

AddDependency(
    TARGET DBOSupreme
    DEPENDENCY Shinzui
)

BuildNow(
    TARGET DBOSupreme
    EXECUTABLE ON
    OUTPUT_NAME DBOSupreme
)

# -[ Build library if tests are enabled
if (DBOS_BUILD_TESTS)
    AddDependency(
        TARGET DBOSupreme_LIB
        DEPENDENCY Shinzui
    )

    BuildNow(
        TARGET DBOSupreme_LIB
        STATIC_LIB ON
        OUTPUT_NAME DBOSupreme_Tests
    )
endif()
