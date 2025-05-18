file(GLOB_RECURSE CORE_SRCS "Src/*.c" "Src/*.cpp")

idf_component_register(
    SRCS ${CORE_SRCS}
    INCLUDE_DIRS "Inc"
    REQUIRES     esp_wifi esp_netif nvs_flash
)
