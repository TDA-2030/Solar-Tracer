
file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_LIST_DIR}/*.c")

idf_component_register(SRCS 
    ${SOURCES}
    
    INCLUDE_DIRS . ws2812_panel

)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format)
target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_LVGL_H_INCLUDE_SIMPLE)
