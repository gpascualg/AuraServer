ResolveExternal(TARGET Shinzui)
if (Shinzui_IS_RESOLVED)
    # -[ Add AuraServer
    add_subdirectory(aura)

    # -[ Unless disabled, add tests
    if (AuraServer_BUILD_TESTS)
        # TODO(gpascualg): REENABLE TESTS
        # add_subdirectory(tests)
    endif()
endif()
