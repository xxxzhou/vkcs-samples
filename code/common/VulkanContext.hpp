#pragma once
#include "VulkanBuffer.hpp"
#include "VulkanCommon.hpp"
#include "VulkanHelper.hpp"
#include "VulkanSwapChain.hpp"
#include "VulkanTexture.hpp"

namespace vkx {
namespace common {

// 把context与swapchain分开,用来Compture Shader做纯GPGPU运行时不需要swapchain
class VKX_COMMON_EXPORT VulkanContext {
   private:
    VkInstance instace;
    std::unique_ptr<VulkanSwapChain> swapchain;
    PhysicalDevice physicalDevice;
    LogicalDevice logicalDevice;

   public:
    VulkanContext(/* args */);
    ~VulkanContext();

   public:
    void CreateContext(const char* appName, uint32_t queueIndex);
};
}  // namespace common
}  // namespace vkx
