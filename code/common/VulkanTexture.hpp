#pragma once

#include "VulkanCommon.hpp"

namespace vkx {
namespace common {
class VKX_COMMON_EXPORT VulkanTexture {    
   private:
    VkImage image;
    VkDeviceMemory memory;
    VkDevice device;
    VkSampler sampler;
    // image可能创建多个mipmap,多层级图像,view针对具体
    VkImageView view;

   public:
    uint32_t width;
    uint32_t height;
    VkFormat format;
   public:
    VulkanTexture();
    ~VulkanTexture();
   public:
    void InitResource(VkDevice _device, uint32_t width, uint32_t height,
                      VkFormat format, VkImageUsageFlags usageFlag,
                      uint32_t memoryTypeIndex, uint8_t *cpuData,
                      uint8_t cpuPitch);
};
}  // namespace common
}  // namespace vkx