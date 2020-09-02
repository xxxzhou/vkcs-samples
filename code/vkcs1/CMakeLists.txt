cmake_minimum_required(VERSION 3.10.2)

set(scr_paths "")
file(GLOB scr_paths "*.cpp" "*.hpp" "*.h")
# aux_source_directory(./ scr_paths)
message(STATUS "xxxx-" ${scr_paths})
set(glsl_sources ${glsl_sources})
foreach(glsl ${glsl_sources})
    glsl_to_spirv(${SAMPLE_NAME}.vert ${SAMPLE_NAME})
endforeach(glsl ${glsl_sources})

# 生成运行程序
add_executable(vkcs1 WIN32 ${scr_paths})
link_directories(${UTILS_NAME} ${Vulkan_LIBRARIES} ${WINLIBS})
message(STATUS "xxxxx-" ${Vulkan_LIBRARIES})
message(STATUS "xxxxx-" ${Vulkan_LIBRARY})
message(STATUS "xxxxx-" ${VULKAN_LOADER_NAME})
message(STATUS "xxxxx-" ${UTILS_NAME})
target_link_libraries(vkcs1 ${UTILS_NAME} ${Vulkan_LIBRARIES} ${WINLIBS} vkxcommon)

vkx_output(vkcs1)
vkx_install(vkcs1)