project(Shinzui)

set(CLUSTER_TYPE map_based)

set(SOURCE_FOLDERS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/cluster/${CLUSTER_TYPE}
    ${CMAKE_CURRENT_SOURCE_DIR}/defs
    ${CMAKE_CURRENT_SOURCE_DIR}/map
)

foreach(folder ${SOURCE_FOLDERS})
    AddToSources(
        TARGET Shinzui
        SRC_PATH ${folder}
        GLOB_SEARCH ".hpp;.cpp"
    )
endforeach()

BuildNow(
    TARGET Shinzui
    STATIC_LIB ON
    OUTPUT_NAME Shinzui
)
