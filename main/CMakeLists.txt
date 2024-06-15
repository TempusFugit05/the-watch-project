file(GLOB WIDGET_SOURCES "widgets/*.cpp" "widgets/*.c")

idf_component_register(SRCS "main.cpp" ${WIDGET_SOURCES} "ds3231.c"
                    INCLUDE_DIRS ".")

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
