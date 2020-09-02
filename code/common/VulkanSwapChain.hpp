#pragma once
#include <memory>

#include "VulkanCommon.hpp"
#include "VulkanTexture.hpp"
#if WIN32
#include "WIN32/Win32Window.hpp"
#endif
#include <functional>

namespace vkx {
namespace common {
class VKX_COMMON_EXPORT VulkanSwapChain {
   private:
    VkSurfaceKHR surface;
    VkInstance instance;
    PhysicalDevice* physicalDevice;
    VkDevice device;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> images;
    std::vector<VkImageView> views;
    VkCommandPool cmdPool;
    std::vector<VkCommandBuffer> cmdBuffers;
#if defined(_WIN32)
    std::unique_ptr<Win32Window> window;
#endif
    VkQueue queue;
    // 图像被获取,可以开始渲染
    VkSemaphore presentComplete;
    // 图像已经渲染,可以呈现
    VkSemaphore renderComplete;
    VkSubmitInfo submitInfo = {};
    /** @brief Pipeline stages used to wait at for graphics queue submissions */
    VkPipelineStageFlags submitPipelineStages =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    std::function<void(uint32_t)> onDraw;

   public:
    uint32_t selectGraphicsIndex = UINT32_MAX;
    uint32_t selectPresentIndex = UINT32_MAX;
    VkFormat format = VK_FORMAT_B8G8R8A8_UNORM;
    uint32_t widht;
    uint32_t height;
    bool vsync = false;
    uint32_t imageCount;
    uint32_t currentBuffer;
    bool bCanDraw = false;

   public:
    VulkanSwapChain(VkInstance _instance, PhysicalDevice* _physicalDevice);
    ~VulkanSwapChain();
    // 没有外部窗口,自己创建
#if defined(_WIN32)
    void InitWindow(HINSTANCE inst, uint32_t _width, uint32_t height,
                    const char* appName);

    LRESULT handleMessage(UINT msg, WPARAM wparam, LPARAM lparam);
#endif
    // 根据窗口创建surface,并返回使用的queueIndex.
#if defined(_WIN32)
    void InitSurface(HINSTANCE inst, HWND windowHandle);
#elif defined(__ANDROID__)
    void InitSurface(ANativeWindow* window);
#endif
    void CreateSwipChain(VkDevice _device,
                         std::function<void(uint32_t)> onDrawAction);
    void Run();

   private:
    void reSwapChain();
};
}  // namespace common
}  // namespace vkx