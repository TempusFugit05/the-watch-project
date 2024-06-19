file(GLOB WIDGET_SOURCES "widgets/src/*.cpp" "widgets/src/*.c")
file(GLOB CORE_SOURCES "core/src/*.cpp")

idf_component_register(SRCS ${CORE_SOURCES} "main.cpp" ${WIDGET_SOURCES} "ds3231/ds3231.c"
                    INCLUDE_DIRS "." "core/include/" "widgets/include/" )

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
