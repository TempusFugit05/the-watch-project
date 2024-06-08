file(GLOB WIDGET_SOURCES "widgets/*.cpp")

idf_component_register(SRCS "main.cpp" ${WIDGET_SOURCES} "widgets/grid/grid.cpp" "ds3231.c"
                    INCLUDE_DIRS ".")