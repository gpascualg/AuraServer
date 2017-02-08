CreateTarget(tests)
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
    DEPENDENCY AuraServer_LIB
)

string(LENGTH ${CMAKE_SOURCE_DIR} BASE_DIR_LEN)
set(DEFINES
    BUILD_TESTS=${SHINZUI_BUILD_TESTS}
    BOOST_ERROR_CODE_HEADER_ONLY
    BOOST_SYSTEM_NO_DEPRECATED
    BOOST_ALL_NO_LIB
    BASE_DIR_LEN=${BASE_DIR_LEN}
)

BuildNow(
    TARGET tests
    EXECUTABLE ON
    DEFINES ${DEFINES}
    OUTPUT_NAME AuraServer_Tests
)
