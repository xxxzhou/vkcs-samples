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

#if defined(__ANDROID__)
// Missing from the NDK
namespace std {
template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
}  // namespace std
#endif

namespace vkx {
namespace common {
VKX_COMMON_EXPORT const std::string getAssetPath();

// errorcode转显示
VKX_COMMON_EXPORT std::string errorString(VkResult errorCode);
// 物理显卡类型
VKX_COMMON_EXPORT std::string physicalDeviceTypeString(
    VkPhysicalDeviceType type);
// 创建一个vulkan实例
VKX_COMMON_EXPORT VkResult createInstance(VkInstance& instance,
                                          const char* appName);
// 得到所有物理显卡
VKX_COMMON_EXPORT VkResult enumerateDevice(
    VkInstance instance, std::vector<vkx::PhysicalDevice>& physicalDevices);
// 创建一个满足条件的逻辑设置,是否使用单独的计算通道
VKX_COMMON_EXPORT VkResult createLogicalDevice(
    vkx::LogicalDevice& device, const vkx::PhysicalDevice& physicalDevice,
    uint32_t queueFamilyIndex, bool bAloneCompute = false);
VKX_COMMON_EXPORT bool getMemoryTypeIndex(
    const vkx::PhysicalDevice& physicalDevice, uint32_t typeBits,
    VkFlags quirementsMaks, uint32_t& index);
VKX_COMMON_EXPORT int32_t getByteSize(VkFormat format);
// Load a SPIR-V shader (binary)
#if defined(__ANDROID__)
VkShaderModule loadShader(AAssetManager* assetManager, const char* fileName,
                          VkDevice device);
#else
VKX_COMMON_EXPORT VkShaderModule loadShader(const char* fileName,
                                            VkDevice device);
#endif

VKX_COMMON_EXPORT void changeLayout(
    VkCommandBuffer command, VkImage image, VkImageLayout oldLayout,
    VkImageLayout newLayout, VkPipelineStageFlags oldStageFlags,
    VkPipelineStageFlags newStageFlags,
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    VkAccessFlags newAccessFlags = 0);

}  // namespace common
}  // namespace vkx
