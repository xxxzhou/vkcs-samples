cmake_minimum_required(VERSION 3.10.2)
# 方案名
project(VKCS_SAMPLES)
# 

# include函数的路径
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
# 查找python执行程序,找不到版本3就停止执行给出错误
find_package(PythonInterp 3 REQUIRED)

# 查找vulkan库引用
message(STATUS "using find_package to locate vulkan")
find_package(Vulkan)
message(STATUS "vulkan found = ${Vulkan_FOUND}")
message(STATUS "vulkan include = ${Vulkan_INCLUDE_DIR}")
message(STATUS "vulkan lib = ${Vulkan_LIBRARY}")
set(API_NAME "Vulkan" CACHE STRING "API name to use when building")
# vulkan版本号现在还是1
set(MAJOR "1")
if(WIN32)  
    string(TOLOWER ${API_NAME} API_LOWERCASE) 
    set (VULKAN_LOADER_NAME "${API_LOWERCASE}-${MAJOR}")
endif()

# 设置本项目的安装目录
include(GNUInstallDirs)
# 如果CMAKE_INSTALL_PREFIX没设置值,用于默认初始化 CMAKE_BINARY_DIR为build目录 CACHE序列化
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT AND WIN32)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "default install path" FORCE)
endif()
if (NOT CMAKE_BUILD_TYPE)
  # CMake defaults to leaving CMAKE_BUILD_TYPE empty. This screws up
  # differentiation between debug and release builds.
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build, options are: None (CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

# 支持方案里工程分文件夹(类似VS里的工程分成几个组,配合SET_PROPERTY(TARGET xxxa PROPERTY FOLDER "xxx") )
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# 设置build的子目录
set(VKX_RUNTIME_DIR ${CMAKE_BINARY_DIR}/bin)
set(VKX_LIBRARY_DIR ${CMAKE_BINARY_DIR}/lib)
# 生成目录
function(vkx_output targetname)
    message(STATUS "---" ${targetname} "---" ${VKX_RUNTIME_DIR})
    set_target_properties(${targetname} PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY ${VKX_LIBRARY_DIR} 
    RUNTIME_OUTPUT_DIRECTORY ${VKX_RUNTIME_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${VKX_LIBRARY_DIR}
    )
endfunction(vkx_output)
# 生成目录复制 ${CMAKE_BUILD_TYPE}
function(vkx_install targetname)
    message(STATUS "----" ${targetname} "----" ${CMAKE_BUILD_TYPE})
    install(TARGETS ${targetname}    
    RUNTIME DESTINATION "${CMAKE_BUILD_TYPE}/bin" CONFIGURATIONS Debug None ""
    LIBRARY DESTINATION "${CMAKE_BUILD_TYPE}/lib" CONFIGURATIONS Debug None ""
    ARCHIVE DESTINATION "${CMAKE_BUILD_TYPE}/lib" CONFIGURATIONS Debug None ""
    )
    install(FILES ${VKX_RUNTIME_DIR}/Debug/${targetname}.pdb
    DESTINATION ${CMAKE_BUILD_TYPE}/bin
    CONFIGURATIONS Debug
    )
endfunction(vkx_install)


# 添加win32平台下的编译符号
if(WIN32)
    # Treat warnings as errors
    #add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/WX>")
    # Disable RTTI
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/GR->")
    # Warn about nested declarations
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/w34456>")
    # Warn about potentially uninitialized variables
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/w34701>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/w34703>")
    # Warn about different indirection types.
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/w34057>")
    # Warn about signed/unsigned mismatch.
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/w34245>")
endif()

# 记录上面的脚本目录
set(SCRIPTS_DIR "${PROJECT_SOURCE_DIR}/scripts")
# glslangValidator用于将GLSL着色转换成SPIR-V
set(GLSLANG_VALIDATOR_NAME "glslangValidator")
message(STATUS "using camke find_program to look for glslangValidator")

if(WIN32)
    # 执行python脚本fetch_glslangvalidator,检查是否有glslang/glslangValidator,没有就自动下载
    execute_process(COMMAND ${PYTHON_EXECUTABLE} ${SCRIPTS_DIR}/fetch_glslangvalidator.py glslang-master-windows-x64-Release.zip)
    set(GLSLANG_VALIDATOR_NAME "glslangValidator.exe")
    # spirv-as用于将SPIR-V可读格式重新编译成汇编代码
    # execute_process(COMMAND ${PYTHON_EXECUTABLE} ${SCRIPTS_DIR}/fetch_spirv_tools.py SPIRV-Tools-master-windows-x64-Release.zip)
    # set(SPIRV_TOOLS_ASSEMBLER_NAME "spirv-as.exe")
endif()

find_program(GLSLANG_VALIDATOR NAMES ${GLSLANG_VALIDATOR_NAME} HINTS "${PROJECT_SOURCE_DIR}/glslang/bin")
message(STATUS "glslangValidator path:" ${GLSLANG_VALIDATOR})

# 添加一个预编译符
add_definitions(-DAPI_NAME="${vulkan}")
# 如果选择clang编译器(android下会用)
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(COMMON_COMPILE_FLAGS "-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers")
    set(COMMON_COMPILE_FLAGS "${COMMON_COMPILE_FLAGS} -fno-strict-aliasing -fno-builtin-memcmp")

    if (CMAKE_COMPILER_IS_GNUCC AND NOT (CMAKE_CXX_COMPILER_VERSION LESS 7.1))
        set(COMMON_COMPILE_FLAGS "${COMMON_COMPILE_FLAGS} -Wimplicit-fallthrough=0")
    endif()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 ${COMMON_COMPILE_FLAGS}")
    # 指定c++11 不带rtti
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_COMPILE_FLAGS} -std=c++11 -fno-rtti")
    if (UNIX)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
    endif()
endif()

# 设定一些变量
set(PYTHON_CMD ${PYTHON_EXECUTABLE})
# set(UTILS_NAME vsamputils)
set(UTILS_DIR "${CMAKE_SOURCE_DIR}/code/utils")
set(COMMON_DIR "${CMAKE_SOURCE_DIR}/code/common")

add_definitions(-DVULKAN_SAMPLES_BASE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

# 检查是否包含utils这个文件夹,这个帮助类包含一些
get_filename_component(GLMINC_PREFIX "${UTILS_DIR}" ABSOLUTE)
message(STATUS "x1-${UTILS_DIR}")
message(STATUS "x2-${GLMINC_PREFIX}")
if(NOT EXISTS ${GLMINC_PREFIX})
    message(FATAL_ERROR "Necessary glm headers do not exist: " ${GLMINC_PREFIX})
endif()

# 当前方案下所有项目,头文件搜索包含目录
include_directories(${UTILS_DIR} ${COMMON_DIR} )
# code目录下的CMakeLists.txt开始执行
add_subdirectory(code)



















