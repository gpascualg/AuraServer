project(tests)

set(SOURCE_FOLDERS
    ${CMAKE_CURRENT_SOURCE_DIR}
)

foreach(folder ${SOURCE_FOLDERS})
    AddToSources(
        TARGET tests
        SRC_PATH ${folder}
        GLOB_SEARCH ".hpp;.cpp"
    )
endforeach()

AddDependency(
    TARGET tests
    DEPENDENCY DBOSupreme_LIB
)

BuildNow(
    TARGET tests
    EXECUTABLE ON
    OUTPUT_NAME DBOSupreme_Tests
)
