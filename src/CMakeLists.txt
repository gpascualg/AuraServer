# -[ Add DBOSupreme
add_subdirectory(aura)

# -[ Unless disabled, add tests
if (AuraServer_BUILD_TESTS)
    add_subdirectory(tests)
endif()
