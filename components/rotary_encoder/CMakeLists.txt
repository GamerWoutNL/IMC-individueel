#idf_component_register(SRCS "rotary.c" INCLUDE_DIRS include)

set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_SRCS "rotary.c" "twist.c")

register_component()