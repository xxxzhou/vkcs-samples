file(GLOB COMMON_SOURCE "*.cpp" "*.hpp" "*.h")

message(STATUS "vkcommon generate")

add_definitions(-DVKX_COMMON_EXPORT_DEFINE)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${Vulkan_INCLUDE_DIR})
add_library(vkxcommon SHARED ${COMMON_SOURCE})

if(WIN32)
    target_link_libraries(vkxcommon ${Vulkan_LIBRARY} ${WINLIBS})
 else(WIN32)
    target_link_libraries(vkxcommon ${Vulkan_LIBRARY})
endif(WIN32)