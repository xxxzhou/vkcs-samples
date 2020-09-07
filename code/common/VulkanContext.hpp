#pragma once
#include "VulkanBuffer.hpp"
#include "VulkanCommon.hpp"
#include "VulkanHelper.hpp"
#include "VulkanTexture.hpp"

namespace vkx {
namespace common {

// 把context与swapchain分开,用来Compture Shader做纯GPGPU运行时不需要swapchain
class VKX_COMMON_EXPORT VulkanContext {
   public:
    VkInstance instace;
    PhysicalDevice physicalDevice;
    LogicalDevice logicalDevice;
    VkQueue computeQueue;
    VkQueue graphicsQueue;

   public:
    VulkanContext(/* args */);
    ~VulkanContext();

   public:
    void CreateInstance(const char* appName);
    void CreateDevice(uint32_t queueIndex, bool bAloneCompute = false);
};
}  // namespace common
}  // namespace vkx
