set(COMPONENT_REQUIRES nvs_flash esp_peripherals)
set(COMPONENT_PRIV_REQUIRES)

set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_SRCS "wifi.c")

register_component()