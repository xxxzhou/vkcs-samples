#pragma once

#include "VulkanCommon.hpp"

namespace vkx {
namespace common {
class VKX_COMMON_EXPORT VulkanTexture {
   private:
    VkDeviceMemory memory;
    VkDevice device;

   public:
    uint32_t width;
    uint32_t height;
    VkFormat format;
    VkSampler sampler;
    // image可能创建多个mipmap,多层级图像,view针对具体
    VkImageView view;
    VkImage image;

   public:
    VulkanTexture();
    ~VulkanTexture();

   public:
    void InitResource(class VulkanContext* context, uint32_t width,
                      uint32_t height, VkFormat format,
                      VkImageUsageFlags usageFlag,
                      VkMemoryPropertyFlagBits memoryFlag,
                      uint8_t* cpuData = nullptr, uint8_t cpuPitch = 0);
};
}  // namespace common
}  // namespace vkx