
file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_LIST_DIR}/*.c")
file(GLOB_RECURSE CPP_SOURCES "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

idf_component_register(SRCS 
    ${SOURCES} ${CPP_SOURCES}

    INCLUDE_DIRS 
    . 
    imu
    wifi
    nmea0183 
    gimbal 
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format -Wno-empty-body -Wno-error=jump-misses-init)


# Add generated header to sources
set(BUILD_TIME_HEADER "${CMAKE_CURRENT_BINARY_DIR}/build_time.h")
if(WIN32)
    # Windows 用 powershell 生成时间字符串
    add_custom_target(build_time ALL
        COMMAND ${CMAKE_COMMAND} -E echo "#pragma once" > ${BUILD_TIME_HEADER}
        COMMAND powershell -Command "Get-Date -Format 'yyyy-MM-dd HH:mm:ss' | ForEach-Object { \"#define BUILD_TIMESTAMP \\\"$_\\\"\" } >> ${BUILD_TIME_HEADER}"
        BYPRODUCTS ${BUILD_TIME_HEADER}
        COMMENT "Generating build_time.h with current time (Windows)"
        VERBATIM
    )
else()
    # Linux/macOS 用 bash date
    add_custom_target(build_time ALL
        COMMAND ${CMAKE_COMMAND} -E echo "#pragma once" > ${BUILD_TIME_HEADER}
        COMMAND bash -c "echo '#define BUILD_TIMESTAMP \"'\"\$(date '+%Y-%m-%d %H:%M:%S')\"'\"' >> ${BUILD_TIME_HEADER}"
        BYPRODUCTS ${BUILD_TIME_HEADER}
        COMMENT "Generating build_time.h with current time (Unix)"
        VERBATIM
    )
endif()
add_dependencies(${COMPONENT_LIB} build_time)

if(CONFIG_EXAMPLE_WEB_DEPLOY_SF)
    set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../front/web-demo")
    if(EXISTS ${WEB_SRC_DIR}/dist)
        spiffs_create_partition_image(www ${WEB_SRC_DIR}/dist FLASH_IN_PROJECT)
    else()
        message(FATAL_ERROR "${WEB_SRC_DIR}/dist doesn't exit. Please run 'npm run build' in ${WEB_SRC_DIR}")
    endif()
endif()
