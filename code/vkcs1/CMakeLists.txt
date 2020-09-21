cmake_minimum_required(VERSION 3.10.2)

set(scr_paths "")
file(GLOB scr_paths "*.cpp" "*.hpp" "*.h")
# aux_source_directory(./ scr_paths)
message(STATUS "xxxx-" ${scr_paths})
file(GLOB glsl_sources "*.comp" "*.hlsl")

# 生成运行程序
message(STATUS "xxxx-" ${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_executable(vkcs1 WIN32 ${scr_paths})
link_directories(${UTILS_NAME} ${Vulkan_LIBRARIES} ${WINLIBS})
message(STATUS "xxxxx-" ${Vulkan_LIBRARIES})
message(STATUS "xxxxx-" ${Vulkan_LIBRARY})
message(STATUS "xxxxx-" ${VULKAN_LOADER_NAME})
message(STATUS "xxxxx-" ${UTILS_NAME})
target_link_libraries(vkcs1 ${Vulkan_LIBRARIES} ${WINLIBS} vkxcommon)

vkx_output(vkcs1)
vkx_install(vkcs1)