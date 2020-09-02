file(GLOB COMMON_SOURCE "*.cpp" "*.hpp" "*.h")

message(STATUS "vkcommon generate")

add_definitions(-DVKX_COMMON_EXPORT_DEFINE)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${Vulkan_INCLUDE_DIR})
message(STATUS "${Vulkan_INCLUDE_DIR}")

if(ANDROID)
    target_include_directories(vkxcommon PRIVATE
        ${ANDROID_NDK}/sources/android/native_app_glue
        ${CMAKE_CURRENT_SOURCE_DIR}/android/vulkan_wrapper)
    file(GLOB PLATFORM_FILES "${CMAKE_CURRENT_SOURCE_DIR}/android/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/android/*.cpp")
endif()
if(WIN32)
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/WIN32")
    file(GLOB PLATFORM_FILES "${CMAKE_CURRENT_SOURCE_DIR}/WIN32/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/WIN32/*.cpp")
endif()

add_library(vkxcommon SHARED ${COMMON_SOURCE} ${PLATFORM_FILES})

if(WIN32)
    target_link_libraries(vkxcommon ${Vulkan_LIBRARY} ${WINLIBS})
 else(WIN32)
    target_link_libraries(vkxcommon ${Vulkan_LIBRARY})
endif(WIN32)

vkx_output(vkxcommon)
vkx_install(vkxcommon)