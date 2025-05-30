file(GLOB_RECURSE TP_SRCS 
    "./http_parser/*.c"
)

idf_component_register(
    SRCS        ${TP_SRCS}
    INCLUDE_DIRS 
        "./http_parser"
)
