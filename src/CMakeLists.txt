# -[ Add shinzui
add_subdirectory(shinzui)

# -[ Add DBOSupreme
add_subdirectory(dbos)

# -[ Unless disabled, add tests
if (DBOS_BUILD_TESTS)
    add_subdirectory(tests)
endif()
