file(GLOB_RECURSE SRC_SRCS "${CMAKE_CURRENT_LIST_DIR}/*.c")

idf_component_register(
    SRCS ${SRC_SRCS}
    INCLUDE_DIRS "../include"
    REQUIRES
        third_party
        esp_wifi
        esp_netif
        nvs_flash
        driver
        esp_http_server
)
