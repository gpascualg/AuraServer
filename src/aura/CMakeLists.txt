CreateTarget(TARGET AuraServer)
project(AuraServer)

set(SOURCE_FOLDERS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/crypto
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

AddDependency(
    TARGET AuraServer
    DEPENDENCY Shinzui
)

BuildNow(
    TARGET AuraServer
    EXECUTABLE ON
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
        OUTPUT_NAME AuraServer_Tests
    )
endif()
