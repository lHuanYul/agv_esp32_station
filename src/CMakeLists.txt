file(GLOB_RECURSE SRC_SRCS "./*.c")

idf_component_register(
    SRCS ${SRC_SRCS}
    INCLUDE_DIRS "../include"
    REQUIRES
        esp_wifi
        esp_netif
        nvs_flash
        driver
)
