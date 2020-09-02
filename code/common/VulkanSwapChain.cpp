#include "VulkanSwapChain.hpp"

#include "VulkanCommon.hpp"
#include "VulkanHelper.hpp"

namespace vkx {
namespace common {
VulkanSwapChain::VulkanSwapChain(VkInstance _instance,
                                 PhysicalDevice* _physicalDevice) {
    this->instance = _instance;
    this->physicalDevice = _physicalDevice;
}

VulkanSwapChain::~VulkanSwapChain() {
    if (swapChain) {
        for (uint32_t i = 0; i < imageCount; i++) {
            vkDestroyImageView(device, views[i], nullptr);
        }
        vkDestroySwapchainKHR(device, swapChain, nullptr);
        vkDestroySemaphore(device, presentComplete, nullptr);
        vkDestroySemaphore(device, renderComplete, nullptr);
        vkFreeCommandBuffers(device, cmdPool,
                             static_cast<uint32_t>(cmdBuffers.size()),
                             cmdBuffers.data());
        vkDestroyCommandPool(device, cmdPool, nullptr);
    }
    if (surface) {
        vkDestroySurfaceKHR(instance, surface, nullptr);
    }
    surface = VK_NULL_HANDLE;
    swapChain = VK_NULL_HANDLE;
}

#if defined(_WIN32)
void VulkanSwapChain::InitWindow(HINSTANCE inst, uint32_t _width,
                                 uint32_t _height, const char* appName) {
    this->widht = _width;
    this->height = _height;
    // 创建窗口
    window = std::make_unique<Win32Window>();
    HWND hwnd = window->InitWindow(inst, widht, height, appName, this);
    // 得到合适的queueIndex
    this->InitSurface(inst, hwnd);
}

LRESULT VulkanSwapChain::handleMessage(UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_SIZE: {
            this->widht = LOWORD(lparam);
            this->height = HIWORD(lparam);
            if (device && swapChain) {
                vkDeviceWaitIdle(device);
                // 重新创建
                this->reSwapChain();
            }
        } break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(window->hwnd, msg, wparam, lparam);
            break;
    }
    return 0;
}

#endif

#if defined(_WIN32)
void VulkanSwapChain::InitSurface(HINSTANCE inst, HWND windowHandle)
#elif defined(__ANDROID__)
void VulkanSwapChain::InitSurface(ANativeWindow* window)
#endif
{
    VkResult ret = VK_SUCCESS;
#if defined(_WIN32)
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hinstance = inst;
    surfaceCreateInfo.hwnd = (HWND)windowHandle;
    VK_CHECK_RESULT(vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo,
                                            nullptr, &surface));
#elif defined(__ANDROID__)
    VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo = {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.window = window;
    VK_CHECK_RESULT(vkCreateAndroidSurfaceKHR(instance, &surfaceCreateInfo,
                                              NULL, &surface));
#endif
    uint32_t queueFamilyCount = physicalDevice->queueFamilyProps.size();
    std::vector<VkBool32> supportsPresent(queueFamilyCount);
    // 检查每个通道的表面是否支持显示
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice->physicalDevice, i,
                                             surface, &supportsPresent[i]);
    }
    // 默认选择支持VK_QUEUE_GRAPHICS_BIT,也能呈现渲染画面的
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if ((physicalDevice->queueFamilyProps[i].queueFlags &
             VK_QUEUE_GRAPHICS_BIT) != 0) {
            if (selectGraphicsIndex == UINT32_MAX) {
                selectGraphicsIndex = i;
            }
            if (supportsPresent[i] == VK_TRUE) {
                selectGraphicsIndex = i;
                selectPresentIndex = i;
                break;
            }
        }
    }
    // 如果没找到二者都支持的
    if (selectPresentIndex == UINT32_MAX) {
        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            if (supportsPresent[i] == VK_TRUE) {
                selectPresentIndex = i;
                break;
            }
        }
    }
    assert(selectGraphicsIndex != UINT32_MAX);
    assert(selectPresentIndex != UINT32_MAX);
    //查找surf支持的显示格式
    uint32_t formatCount;
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(
        physicalDevice->physicalDevice, surface, &formatCount, nullptr));
    std::vector<VkSurfaceFormatKHR> surfFormats(formatCount);
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(
        physicalDevice->physicalDevice, surface, &formatCount,
        surfFormats.data()));
    if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED) {
        format = VK_FORMAT_B8G8R8A8_UNORM;
    } else {
        assert(formatCount >= 1);
        format = surfFormats[0].format;
    }
}

void VulkanSwapChain::CreateSwipChain(
    VkDevice _device, std::function<void(uint32_t)> onDrawAction) {
    this->device = _device;
    this->onDraw = onDrawAction;
    // 创建semaphore 与 submitInfo,同步渲染与命令执行的顺序
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    // 确保显示图像后再提交渲染命令,vkAcquireNextImageKHR用来presentComplete使presentComplete上锁
    VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr,
                                      &presentComplete));
    // 确保提交与执行命令后,才能执行显示图像
    VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr,
                                      &renderComplete));
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = &submitPipelineStages;
    submitInfo.waitSemaphoreCount = 1;
    // 用于指定队列开始执行前需要等待的信号量，以及需要等待的管线阶段
    submitInfo.pWaitSemaphores = &presentComplete;
    submitInfo.signalSemaphoreCount = 1;
    // 用于缓冲命令执行结束后发出信号的信号量对象
    submitInfo.pSignalSemaphores = &renderComplete;
    // 创建swapchain以及对应的image
    this->reSwapChain();
    // 得到当前使用的queue
    vkGetDeviceQueue(device, selectGraphicsIndex, 0, &queue);
    // reSwapChain主要是重新生成由widht/height改变的资源 cmdPool
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = selectGraphicsIndex;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK_RESULT(
        vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &cmdPool));
    // imageCount一般来说不会变动
    cmdBuffers.resize(imageCount);
    VkCommandBufferAllocateInfo cmdBufInfo = {};
    cmdBufInfo.commandPool = cmdPool;
    cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufInfo.commandBufferCount = cmdBuffers.size();
    VK_CHECK_RESULT(
        vkAllocateCommandBuffers(device, &cmdBufInfo, cmdBuffers.data()));
}

void VulkanSwapChain::reSwapChain() {
    this->bCanDraw = false;
    VkSwapchainKHR oldSwapchain = swapChain;
    // Get physical device surface properties and formats
    VkSurfaceCapabilitiesKHR surfCapabilities;
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physicalDevice->physicalDevice, surface, &surfCapabilities));

    uint32_t presentModeCount;
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(
        physicalDevice->physicalDevice, surface, &presentModeCount, nullptr));

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(
        physicalDevice->physicalDevice, surface, &presentModeCount,
        presentModes.data()));

    VkExtent2D swapchainExtent;
    // width and height are either both 0xFFFFFFFF, or both not 0xFFFFFFFF.
    if (surfCapabilities.currentExtent.width == 0xFFFFFFFF) {
        // If the surface size is undefined, the size is set to
        // the size of the images requested.
        swapchainExtent.width = widht;
        swapchainExtent.height = height;
        if (swapchainExtent.width < surfCapabilities.minImageExtent.width) {
            swapchainExtent.width = surfCapabilities.minImageExtent.width;
        } else if (swapchainExtent.width >
                   surfCapabilities.maxImageExtent.width) {
            swapchainExtent.width = surfCapabilities.maxImageExtent.width;
        }

        if (swapchainExtent.height < surfCapabilities.minImageExtent.height) {
            swapchainExtent.height = surfCapabilities.minImageExtent.height;
        } else if (swapchainExtent.height >
                   surfCapabilities.maxImageExtent.height) {
            swapchainExtent.height = surfCapabilities.maxImageExtent.height;
        }
    } else {
        // If the surface size is defined, the swap chain size must match
        swapchainExtent = surfCapabilities.currentExtent;
    }
    this->widht = swapchainExtent.width;
    this->height = swapchainExtent.height;
    // present需要支持FIFO
    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    if (!vsync) {
        for (size_t i = 0; i < presentModeCount; i++) {
            if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
                swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
            if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) &&
                (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)) {
                swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
        }
    }
    uint32_t desiredNumberOfSwapchainImages =
        surfCapabilities.minImageCount + 1;
    if ((surfCapabilities.maxImageCount > 0) &&
        (desiredNumberOfSwapchainImages > surfCapabilities.maxImageCount)) {
        desiredNumberOfSwapchainImages = surfCapabilities.maxImageCount;
    }
    // Find the transformation of the surface
    VkSurfaceTransformFlagsKHR preTransform;
    if (surfCapabilities.supportedTransforms &
        VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        // We prefer a non-rotated transform
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        preTransform = surfCapabilities.currentTransform;
    }
    // Find a supported composite alpha mode - one of these is guaranteed to be
    // set
    VkCompositeAlphaFlagBitsKHR compositeAlpha =
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    VkCompositeAlphaFlagBitsKHR compositeAlphaFlags[4] = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };
    for (uint32_t i = 0;
         i < sizeof(compositeAlphaFlags) / sizeof(compositeAlphaFlags[0]);
         i++) {
        if (surfCapabilities.supportedCompositeAlpha & compositeAlphaFlags[i]) {
            compositeAlpha = compositeAlphaFlags[i];
            break;
        }
    }
    // crate swapchain
    VkSwapchainCreateInfoKHR swapchainInfo = {};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.pNext = nullptr;
    swapchainInfo.surface = surface;
    swapchainInfo.minImageCount = desiredNumberOfSwapchainImages;
    swapchainInfo.imageFormat = format;
    swapchainInfo.imageExtent.width = swapchainExtent.width;
    swapchainInfo.imageExtent.height = swapchainExtent.height;
    swapchainInfo.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
    swapchainInfo.compositeAlpha = compositeAlpha;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.presentMode = swapchainPresentMode;
    swapchainInfo.oldSwapchain = VK_NULL_HANDLE;
    swapchainInfo.clipped = true;
    swapchainInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.queueFamilyIndexCount = 0;
    swapchainInfo.pQueueFamilyIndices = nullptr;
    uint32_t queueFamilyIndices[2] = {selectGraphicsIndex, selectPresentIndex};
    if (selectGraphicsIndex != selectPresentIndex) {
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainInfo.queueFamilyIndexCount = 2;
        swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    VK_CHECK_RESULT(
        vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &swapChain));
    // 清理上一次资源
    if (oldSwapchain) {
        for (uint32_t i = 0; i < imageCount; i++) {
            vkDestroyImageView(device, views[i], nullptr);
        }
        vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
    }
    VK_CHECK_RESULT(
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr));
    images.resize(imageCount);
    VK_CHECK_RESULT(
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, images.data()));
    views.resize(imageCount);
    cmdBuffers.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++) {
        VkImageViewCreateInfo colorAttachmentView = {};
        colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        colorAttachmentView.pNext = nullptr;
        colorAttachmentView.format = format;
        colorAttachmentView.components = {
            VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
            VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
        colorAttachmentView.subresourceRange.aspectMask =
            VK_IMAGE_ASPECT_COLOR_BIT;
        colorAttachmentView.subresourceRange.baseMipLevel = 0;
        colorAttachmentView.subresourceRange.levelCount = 1;
        colorAttachmentView.subresourceRange.baseArrayLayer = 0;
        colorAttachmentView.subresourceRange.layerCount = 1;
        colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        colorAttachmentView.flags = 0;
        colorAttachmentView.image = images[i];
        VK_CHECK_RESULT(vkCreateImageView(device, &colorAttachmentView, nullptr,
                                          &views[i]));
    }
    this->bCanDraw = true;
}

void VulkanSwapChain::Run() {
#if defined(_WIN32)
    while (true) {
        bool quit = false;
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                quit = true;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (quit) {
            break;
        }
        if (bCanDraw && onDraw) {
            // 得到下一次渲染图片
            VkResult result = vkAcquireNextImageKHR(
                device, swapChain, UINT64_MAX, presentComplete,
                (VkFence) nullptr, &currentBuffer);
            if (result == VK_SUCCESS) {
                onDraw(currentBuffer);
            }
            // 提交缓冲区命令,执行完后发送信号给renderComplete
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &cmdBuffers[currentBuffer];
            // 这句应该会给presentComplete上锁,等到命令完成后发送信号
            VK_CHECK_RESULT(
                vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
            // 提交渲染呈现到屏幕
            VkPresentInfoKHR presentInfo = {};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.pNext = nullptr;
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &swapChain;
            presentInfo.pImageIndices = &currentBuffer;
            if (renderComplete != VK_NULL_HANDLE) {
                // renderComplete等命令执行后,发送信号量
                presentInfo.pWaitSemaphores = &renderComplete;
                presentInfo.waitSemaphoreCount = 1;
            }
            // 提交渲染呈现到屏幕
            result = vkQueuePresentKHR(queue, &presentInfo);
            // 等待指令队列结束
            VK_CHECK_RESULT(vkQueueWaitIdle(queue));
        }
    }
#endif
}

}  // namespace common
}  // namespace vkx