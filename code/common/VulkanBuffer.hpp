#pragma once
#include "VulkanCommon.hpp"

namespace vkx {
namespace common {
// gpu buffer主要用于保存GPU数据,几种主要场景
enum BufferUsage {
    VKS_BUFFER_USAGE_OTHER,
    // 1 用于和CPU交互,如UBO 可能需要一直更新
    VKS_BUFFER_USAGE_STORE,
    // 2 用于GPU/GPU计算中的缓存数据,并不需要和CPU交互
    VKS_BUFFER_USAGE_PROGRAM,
    // 3 一次和CPU交互,后续一直用于GPU中,如纹理,模型VBO
    VKS_BUFFER_USAGE_ONESTORE,
};

// (CPU可写)VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
// (unmap后GPU最新)VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
// (CPU不可写,Computer shader)(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
class VulkanBuffer {
   private:
    /* data */
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDevice device;
    VkBufferView view;

   public:
    VulkanBuffer();
    ~VulkanBuffer();

   public:
    void InitResource(VkDevice _device, uint32_t dataSize, VkFormat viewFormat,
                      VkBufferUsageFlagBits usageFlag, uint32_t memoryTypeIndex,
                      uint8_t *cpuData = nullptr);
};
}  // namespace common
}  // namespace vkx
