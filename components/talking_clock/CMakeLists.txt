set(COMPONENT_REQUIRES audio_pipeline audio_stream esp_peripherals wifi esp32-smbus esp32-i2c-lcd1602 timesync input_key_service ssd1306 led_management)
set(COMPONENT_PRIV_REQUIRES)

set(COMPONENT_SRCS "talking_clock.c")
set(COMPONENT_ADD_INCLUDEDIRS include)

register_component()