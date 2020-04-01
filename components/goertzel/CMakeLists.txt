#idf_component_register(SRCS "" INCLUDE_DIRS include)

set(COMPONENT_REQUIRES audio_pipeline audio_stream audio_board audio_hal esp_peripherals radio)
set(COMPONENT_PRIV_REQUIRES)
set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_SRCS "goertzel.c")
register_component()
