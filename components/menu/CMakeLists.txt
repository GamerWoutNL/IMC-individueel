#idf_component_register(SRCS "menu.c" INCLUDE_DIRS include)

set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_SRCS "menu.c")
set(COMPONENT_PRIV_REQUIRES "esp32-i2c-lcd1602" "esp32-smbus")

register_component()