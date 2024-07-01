file(GLOB CORE_SOURCES "core/src/*.c")
file(GLOB DS3231_SOURCES "sensor_drivers/ds3231/ds3231.c")
file(GLOB WIDGET_SOURCES "widgets/src/*.c")

idf_component_register(SRCS "main.c" ${CORE_SOURCES} ${WIDGET_SOURCES} ${DS3231_SOURCES} "sensor_drivers/max30102/max30102.c"
                    INCLUDE_DIRS "." "core/include/" "widgets/include/" "sensor_drivers/ds3231/" "sensor_drivers/max30102/")
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")