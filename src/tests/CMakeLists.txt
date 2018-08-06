CreateTarget(tests)
project(tests)

AddToSources(
    TARGET tests
    GLOB_SEARCH ".hpp;.cpp"
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}
)

AddDependency(
    TARGET tests
    DEPENDENCY AuraServer_Lib
)

string(LENGTH ${CMAKE_SOURCE_DIR} BASE_DIR_LEN)
AddToDefinitions(
    TARGET tests
    DEFINITIONS
        BUILD_TESTS=${SHINZUI_BUILD_TESTS}
        BOOST_ERROR_CODE_HEADER_ONLY
        BOOST_SYSTEM_NO_DEPRECATED
        BOOST_ALL_NO_LIB
        _WIN32_WINNT=0x0501
        PRIVATE
            BASE_DIR_LEN=${BASE_DIR_LEN}
)

BuildNow(
    TARGET tests
    EXECUTABLE ON
    OUTPUT_NAME AuraServer_Tests
)
