file(GLOB_RECURSE TP_SRCS 
    "${CMAKE_CURRENT_LIST_DIR}/http_parser/*.c"
)

idf_component_register(
    SRCS        ${TP_SRCS}
    INCLUDE_DIRS
        "."
        "http_parser"
)
