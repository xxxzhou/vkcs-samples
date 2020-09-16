
file(GLOB COMMON_SRC "../../code/common/*.cpp" "../../code/common/*.hpp")
set(COMMON_SRC ${COMMON_SRC} "../../code/common/android/vulkan_wrapper.cpp")

add_library(libcommon STATIC ${COMMON_SRC})

include_directories(${ANDROID_NDK}/sources/android/native_app_glue)

target_link_libraries(libcommon android log)