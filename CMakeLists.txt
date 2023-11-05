set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)

set(requires "esp_wifi" "esp_netif" "wifi_provisioning" "esp_eth" "esp_http_server" "esp_http_client" "wpa_supplicant")

file(GLOB USER_SRCS ./main/*.cc)

idf_component_register(SRCS ${USER_SRCS}
        INCLUDE_DIRS "./main"
        PRIV_REQUIRES driver
        REQUIRES ${requires}
)
