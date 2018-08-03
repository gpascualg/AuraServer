CreateTarget(TARGET AuraServer)
CreateTarget(TARGET AuraServer_Lib)

project(AuraServer)

set(SOURCE_FOLDERS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/server
    ${CMAKE_CURRENT_SOURCE_DIR}/client
    ${CMAKE_CURRENT_SOURCE_DIR}/entity
)

foreach (target "AuraServer" "AuraServer_Lib")
    AddToSources(
        TARGET ${target}
        GLOB_SEARCH ".hpp;.cpp"
        SOURCES ${SOURCE_FOLDERS}
    )

    AddToIncludes(
        TARGET ${target}
        INCLUDES 
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    # -[ Build executable
    string(LENGTH ${CMAKE_SOURCE_DIR} BASE_DIR_LEN)
    AddToDefinitions(
        TARGET ${target}
        DEFINITIONS
            BUILD_TESTS=${SHINZUI_BUILD_TESTS}
            BOOST_ERROR_CODE_HEADER_ONLY
            BOOST_SYSTEM_NO_DEPRECATED
            BOOST_ALL_NO_LIB
            BASE_DIR_LEN=${BASE_DIR_LEN}
            _WIN32_WINNT=0x0501
    )

    AddDependency(
        TARGET ${target}
        DEPENDENCY Shinzui
    )
endforeach()


BuildNow(
    TARGET AuraServer
    EXECUTABLE ON
    OUTPUT_NAME AuraServer
)

# -[ Build library if tests are enabled
if (AuraServer_BUILD_TESTS)
    BuildNow(
        TARGET AuraServer_Lib
        STATIC_LIB ON
        OUTPUT_NAME AuraServer_TestLib
    )
endif()
