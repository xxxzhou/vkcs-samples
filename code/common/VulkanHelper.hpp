#pragma once
#include "VulkanCommon.hpp"

#if defined(__ANDROID__)
#define LOGI(...) \
    ((void)__android_log_print(ANDROID_LOG_INFO, "vkx", __VA_ARGS__))
#define LOGE(...) \
    ((void)__android_log_print(ANDROID_LOG_ERROR, "vkx", __VA_ARGS__))
#define VK_CHECK_RESULT(f)                                                   \
    {                                                                        \
        VkResult res = (f);                                                  \
        if (res != VK_SUCCESS) {                                             \
            LOGE("Fatal : VkResult is \" %s \" in %s at line %d",            \
                 vkx::common::errorString(res).c_str(), __FILE__, __LINE__); \
            assert(res == VK_SUCCESS);                                       \
        }                                                                    \
    }
#else
#define VK_CHECK_RESULT(f)                                                     \
    {                                                                          \
        VkResult res = (f);                                                    \
        if (res != VK_SUCCESS) {                                               \
            std::cout << "Fatal : VkResult is \""                              \
                      << vkx::common::errorString(res) << "\" in " << __FILE__ \
                      << " at line " << __LINE__ << "\n";                      \
            assert(res == VK_SUCCESS);                                         \
        }                                                                      \
    }
#endif

namespace vkx {
namespace common {
// errorcode转显示
VKX_COMMON_EXPORT std::string errorString(VkResult errorCode);
// 物理显卡类型
VKX_COMMON_EXPORT std::string physicalDeviceTypeString(
    VkPhysicalDeviceType type);
// 创建一个vulkan实例
VKX_COMMON_EXPORT VkResult createInstance(VkInstance instance,
                                          const char* appName);
// 得到所有物体显卡
VKX_COMMON_EXPORT VkResult enumerateDevice(
    VkInstance instance, std::vector<vkx::PhysicalDevice>& physicalDevices);
// 创建一个满足的逻辑设置,是否使用单独的计算通道
VKX_COMMON_EXPORT VkResult createLogicalDevice(
    vkx::LogicalDevice& device, const vkx::PhysicalDevice& physicalDevice,
    bool bAloneCompute = false);
//

}  // namespace common
}  // namespace vkx
