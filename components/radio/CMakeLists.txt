set(COMPONENT_REQUIRES audio_pipeline audio_stream esp_peripherals wifi led_management)
set(COMPONENT_PRIV_REQUIRES)

set(COMPONENT_SRCS "radio.c")
set(COMPONENT_ADD_INCLUDEDIRS include)

register_component()