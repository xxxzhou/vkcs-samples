
cmake_minimum_required(VERSION 2.8.11)

set(SRC_DIR ../../code)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -DVK_USE_PLATFORM_ANDROID_KHR -DVK_NO_PROTOTYPES")

add_library(vkcs1 SHARED ../../code/vkcs1/cs.cpp)
add_library(native-app-glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

include_directories(${SRC_DIR}/common)
include_directories(${ANDROID_NDK}/sources/android/native_app_glue)

add_subdirectory(../base base_binary)

target_link_libraries(vkcs1 android native-app-glue libcommon log)



