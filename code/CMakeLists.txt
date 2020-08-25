cmake_minimum_required(VERSION 3.10.2)

# 把相应的glsl代码转化成SPIR-V代码
macro(glsl_to_spirv src basename)
    # 调用python脚本执行glslangValidator 编译文件 生成文件(很多变量来自根cmake文件)
    add_custom_command(OUTPUT ${src}.h COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/generate_spirv.py 
    ${CMAKE_CURRENT_SOURCE_DIR}/${basename}/${src} ${CMAKE_CURRENT_BINARY_DIR}/${src}.h ${GLSLANG_VALIDATOR} false 
    DEPENDS ${CMAKE_SOURCE_DIR}/scripts/generate_spirv.py 
    ${CMAKE_CURRENT_SOURCE_DIR}/${basename}/${src} ${CMAKE_CURRENT_BINARY_DIR}/${src}.h ${GLSLANG_VALIDATOR})
endmacro(glsl_to_spirv src basename)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # 添加用于不同平台窗口初始化的预编译指令
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR -DWIN32_LEAN_AND_MEAN)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR)
else()
    message(FATAL_ERROR "unsupported platform.")
endif()

# 项目存放的资源目录
set(SAMPLES_DATA_DIR ${SAMPLES_DATA_DIR} "${CMAKE_SOURCE_DIR}/code/data")
# 收集项目的shader文件
set(SHADER_FILES ${SHADER_FILES} "")

# 添加头文件搜索包含目录(资源/utils/vulkan/shader生成目录)
include_directories(${SAMPLES_DATA_DIR} ${GLMINC_PREFIX} ${Vulkan_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

# 执行vkcs1目录下的cmakelists
add_subdirectory(vkcs1)

add_subdirectory(utils)



