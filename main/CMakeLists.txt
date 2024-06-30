file(GLOB WIDGET_SOURCES "widgets/src/*.cpp" "widgets/src/*.c")
file(GLOB CORE_SOURCES "core/src/*.cpp" "core/src/*.c")
file(GLOB DS3231_SOURCES "ds3231/*.c")

idf_component_register(SRCS "main.cpp" ${CORE_SOURCES} ${WIDGET_SOURCES} ${DS3231_SOURCES}
                    INCLUDE_DIRS "." "core/include/" "widgets/include/" "ds3231/")

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")