#include "VulkanWindow.hpp"

#include <array>

#include "VulkanCommon.hpp"
#include "VulkanContext.hpp"
#include "VulkanHelper.hpp"
namespace vkx {
namespace common {
VulkanWindow::VulkanWindow(class VulkanContext* _context) {
    this->context = _context;
    this->instance = context->instace;
    this->physicalDevice = context->physicalDevice.physicalDevice;
}

VulkanWindow::~VulkanWindow() {
    if (swapChain) {
        for (uint32_t i = 0; i < imageCount; i++) {
            vkDestroyImageView(device, views[i], nullptr);
            vkDestroyFramebuffer(device, frameBuffers[i], nullptr);
        }
        vkDestroySwapchainKHR(device, swapChain, nullptr);
        vkDestroySemaphore(device, presentComplete, nullptr);
        vkDestroySemaphore(device, renderComplete, nullptr);
        vkDestroyFence(device, presentFence, nullptr);
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
void VulkanWindow::InitWindow(HINSTANCE inst, uint32_t _width,
                                 uint32_t _height, const char* appName) {
    this->width = _width;
    this->height = _height;
    // 创建窗口
    window = std::make_unique<Win32Window>();
    HWND hwnd = window->InitWindow(inst, width, height, appName, this);
    // 得到合适的queueIndex
    this->InitSurface(inst, hwnd);
}

LRESULT VulkanWindow::handleMessage(UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_SIZE: {
            this->width = LOWORD(lparam);
            this->height = HIWORD(lparam);
            if (device && swapChain) {
                vkDeviceWaitIdle(device);
                // 重新创建
                this->reSwapChain();
                //重新创建frameBuffer
                for (uint32_t i = 0; i < frameBuffers.size(); i++) {
                    vkDestroyFramebuffer(device, frameBuffers[i], nullptr);
                }
                frameBuffers.clear();
                this->createFrameBuffer();
                //更新
                renderPassBeginInfo.renderArea.extent = {width, height};
                viewport.width = width;
                viewport.height = height;
                scissor.extent = {width, height};
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
void VulkanWindow::InitSurface(HINSTANCE inst, HWND windowHandle)
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
    uint32_t queueFamilyCount = context->physicalDevice.queueFamilyProps.size();
    std::vector<VkBool32> supportsPresent(queueFamilyCount);
    // 检查每个通道的表面是否支持显示
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface,
                                             &supportsPresent[i]);
    }
    // 默认选择支持VK_QUEUE_GRAPHICS_BIT,也能呈现渲染画面的
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if ((context->physicalDevice.queueFamilyProps[i].queueFlags &
             VK_QUEUE_GRAPHICS_BIT) != 0) {
            if (graphicsQueueIndex == UINT32_MAX) {
                graphicsQueueIndex = i;
            }
            if (supportsPresent[i] == VK_TRUE) {
                graphicsQueueIndex = i;
                presentQueueIndex = i;
                break;
            }
        }
    }
    // 如果没找到二者都支持的
    if (presentQueueIndex == UINT32_MAX) {
        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            if (supportsPresent[i] == VK_TRUE) {
                presentQueueIndex = i;
                break;
            }
        }
    }
    assert(graphicsQueueIndex != UINT32_MAX);
    assert(presentQueueIndex != UINT32_MAX);
    //查找surf支持的显示格式
    uint32_t formatCount;
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(
        physicalDevice, surface, &formatCount, nullptr));
    std::vector<VkSurfaceFormatKHR> surfFormats(formatCount);
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(
        physicalDevice, surface, &formatCount, surfFormats.data()));
    if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED) {
        format = VK_FORMAT_B8G8R8A8_UNORM;
    } else {
        assert(formatCount >= 1);
        format = surfFormats[0].format;
    }
}

void VulkanWindow::CreateSwipChain(
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
    // 创建cpu-gpu通知
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // 默认是有信息
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(device, &fenceInfo, nullptr, &presentFence);
    // 用于渲染队列里确定等待与发送信号
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
    vkGetDeviceQueue(device, graphicsQueueIndex, 0, &graphicsQueue);
    if (graphicsQueueIndex == presentQueueIndex) {
        presentQueue = graphicsQueue;
    } else {
        vkGetDeviceQueue(device, presentQueueIndex, 0, &presentQueue);
    }
    // reSwapChain主要是重新生成由widht/height改变的资源 cmdPool
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = graphicsQueueIndex;
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
    // 创建窗口用的renderpass
    this->createRenderPass();
    // 创建窗口需要的framebuffer
    this->createFrameBuffer();
    // 创建cmdBufferBeginInfo
    cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    // 创建renderpassbeginInfo
    // VkClearValue clearValues[2];
    clearValues[0].color = {{0.0f, 1.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.renderArea.extent = {width, height};
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;
    // viewport
    viewport.width = (float)width;
    viewport.height = (float)height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    // scissor
    scissor.extent = {width, height};
    scissor.offset = {0, 0};
}

void VulkanWindow::reSwapChain() {
    this->bCanDraw = false;
    VkSwapchainKHR oldSwapchain = swapChain;
    // Get physical device surface properties and formats
    VkSurfaceCapabilitiesKHR surfCapabilities;
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physicalDevice, surface, &surfCapabilities));

    uint32_t presentModeCount;
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(
        physicalDevice, surface, &presentModeCount, nullptr));

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(
        physicalDevice, surface, &presentModeCount, presentModes.data()));

    VkExtent2D swapchainExtent;
    // width and height are either both 0xFFFFFFFF, or both not 0xFFFFFFFF.
    if (surfCapabilities.currentExtent.width == 0xFFFFFFFF) {
        // If the surface size is undefined, the size is set to
        // the size of the images requested.
        swapchainExtent.width = width;
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
    this->width = swapchainExtent.width;
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
    swapchainInfo.oldSwapchain = swapChain;
    swapchainInfo.clipped = true;
    swapchainInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.queueFamilyIndexCount = 0;
    swapchainInfo.pQueueFamilyIndices = nullptr;
    uint32_t queueFamilyIndices[2] = {graphicsQueueIndex, presentQueueIndex};
    // 如果二个不同队列,我们需要CONCURRENT共享不同队列传输
    if (graphicsQueueIndex != presentQueueIndex) {
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainInfo.queueFamilyIndexCount = 2;
        swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    VK_CHECK_RESULT(
        vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &swapChain));
    // depth
    depthTex = std::make_unique<VulkanTexture>();
    depthTex->InitResource(context, width, height, depthFormat,
                           VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (oldSwapchain != VK_NULL_HANDLE) {
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
    frameBuffers.resize(imageCount);
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

void VulkanWindow::createRenderPass() {
    std::array<VkAttachmentDescription, 2> attachments = {};
    // Color attachment
    attachments[0].format = format;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    // Depth attachment
    attachments[1].format = depthFormat;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorReference = {};
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthReference = {};
    depthReference.attachment = 1;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorReference;
    subpassDescription.pDepthStencilAttachment = &depthReference;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = nullptr;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments = nullptr;
    subpassDescription.pResolveAttachments = nullptr;

    // Subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDescription;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    VK_CHECK_RESULT(
        vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
}

void VulkanWindow::createFrameBuffer() {
    VkImageView attachments[2];
    // Depth/Stencil attachment is the same for all frame buffers
    attachments[1] = depthTex->view;
    VkFramebufferCreateInfo frameBufferCreateInfo = {};
    frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.pNext = NULL;
    frameBufferCreateInfo.renderPass = renderPass;
    frameBufferCreateInfo.attachmentCount = 2;
    // 需要满足renderpass创建指定的VkAttachmentDescription
    frameBufferCreateInfo.pAttachments = attachments;
    frameBufferCreateInfo.width = width;
    frameBufferCreateInfo.height = height;
    frameBufferCreateInfo.layers = 1;
    // Create frame buffers for every swap chain image
    frameBuffers.resize(imageCount);
    for (uint32_t i = 0; i < frameBuffers.size(); i++) {
        attachments[0] = views[i];
        VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo,
                                            nullptr, &frameBuffers[i]));
    }
}

void VulkanWindow::beginFrame() {
    vkBeginCommandBuffer(cmdBuffers[currentIndex], &cmdBufferBeginInfo);
    vkCmdSetViewport(cmdBuffers[currentIndex], 0, 1, &viewport);
    vkCmdSetScissor(cmdBuffers[currentIndex], 0, 1, &scissor);
    renderPassBeginInfo.framebuffer = frameBuffers[currentIndex];
    // renderpass关联渲染目标
    vkCmdBeginRenderPass(cmdBuffers[currentIndex], &renderPassBeginInfo,
                         VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanWindow::endFrame() {
    vkCmdEndRenderPass(cmdBuffers[currentIndex]);
    vkEndCommandBuffer(cmdBuffers[currentIndex]);
}

void VulkanWindow::Run() {
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
            // // 等待开门,默认门是开的
            // VK_CHECK_RESULT(
            //     vkWaitForFences(device, 1, &presentFence, true, UINT64_MAX));
            // // 关门 VkFence不同于VkSemaphore,需要手动reset.
            // VK_CHECK_RESULT(vkResetFences(device, 1, &presentFence));
            // 发送信号给presentComplete
            VkResult result = vkAcquireNextImageKHR(
                device, swapChain, UINT64_MAX, presentComplete,
                (VkFence) nullptr, &currentIndex);
            if (result == VK_SUCCESS) {
                beginFrame();
                onDraw(currentIndex);
                endFrame();
            }
            // 提交缓冲区命令,执行完后发送信号给renderComplete
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &cmdBuffers[currentIndex];
            // 等到命令完成后发送信号renderComplete
            VK_CHECK_RESULT(
                vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
            // 提交渲染呈现到屏幕
            VkPresentInfoKHR presentInfo = {};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.pNext = nullptr;
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &swapChain;
            presentInfo.pImageIndices = &currentIndex;
            // 等待renderComplete
            presentInfo.pWaitSemaphores = &renderComplete;
            presentInfo.waitSemaphoreCount = 1;
            // 提交渲染呈现到屏幕
            result = vkQueuePresentKHR(presentQueue, &presentInfo);
            assert(result == VK_SUCCESS);
            // // presentFence开门(发送信号量)
            // VK_CHECK_RESULT(
            //     vkQueueSubmit(presentQueue, 0, nullptr, presentFence));
        }
    }
#endif
}

}  // namespace common
}  // namespace vkx