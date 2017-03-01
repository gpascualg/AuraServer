CreateTarget(TARGET AuraServer)
project(AuraServer)

set(SOURCE_FOLDERS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/server
    ${CMAKE_CURRENT_SOURCE_DIR}/client
)

foreach(folder ${SOURCE_FOLDERS})
    AddToSources(
        TARGET AuraServer
        SRC_PATH ${folder}
        GLOB_SEARCH ".hpp;.cpp"
    )

    AddToSources(
        TARGET AuraServer_LIB
        SRC_PATH ${folder}
        GLOB_SEARCH ".hpp;.cpp"
    )
endforeach()

# -[ Build executable
string(LENGTH ${CMAKE_SOURCE_DIR} BASE_DIR_LEN)
set(DEFINES
    BUILD_TESTS=${SHINZUI_BUILD_TESTS}
    BOOST_ERROR_CODE_HEADER_ONLY
    BOOST_SYSTEM_NO_DEPRECATED
    BOOST_ALL_NO_LIB
    BASE_DIR_LEN=${BASE_DIR_LEN}
    _WIN32_WINNT=0x0501
)

AddDependency(
    TARGET AuraServer
    DEPENDENCY Shinzui
)

if (UNIX)
    AddDependency(
        TARGET AuraServer
        DEPENDENCY pthread
    )
endif()

BuildNow(
    TARGET AuraServer
    EXECUTABLE ON
    DEFINES ${DEFINES}
    OUTPUT_NAME AuraServer
)

# -[ Build library if tests are enabled
if (AuraServer_BUILD_TESTS)
    AddDependency(
        TARGET AuraServer_LIB
        DEPENDENCY Shinzui
    )

    BuildNow(
        TARGET AuraServer_LIB
        STATIC_LIB ON
        DEFINES ${DEFINES}
        OUTPUT_NAME AuraServer_Tests
    )
endif()
