#include "VulkanHelper.hpp"

namespace vkx {
namespace common {
std::string errorString(VkResult errorCode) {
    switch (errorCode) {
#define STR(r)   \
    case VK_##r: \
        return #r
        STR(NOT_READY);
        STR(TIMEOUT);
        STR(EVENT_SET);
        STR(EVENT_RESET);
        STR(INCOMPLETE);
        STR(ERROR_OUT_OF_HOST_MEMORY);
        STR(ERROR_OUT_OF_DEVICE_MEMORY);
        STR(ERROR_INITIALIZATION_FAILED);
        STR(ERROR_DEVICE_LOST);
        STR(ERROR_MEMORY_MAP_FAILED);
        STR(ERROR_LAYER_NOT_PRESENT);
        STR(ERROR_EXTENSION_NOT_PRESENT);
        STR(ERROR_FEATURE_NOT_PRESENT);
        STR(ERROR_INCOMPATIBLE_DRIVER);
        STR(ERROR_TOO_MANY_OBJECTS);
        STR(ERROR_FORMAT_NOT_SUPPORTED);
        STR(ERROR_SURFACE_LOST_KHR);
        STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
        STR(SUBOPTIMAL_KHR);
        STR(ERROR_OUT_OF_DATE_KHR);
        STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
        STR(ERROR_VALIDATION_FAILED_EXT);
        STR(ERROR_INVALID_SHADER_NV);
#undef STR
        default:
            return "UNKNOWN_ERROR";
    }
}

std::string physicalDeviceTypeString(VkPhysicalDeviceType type) {
    switch (type) {
#define STR(r)                        \
    case VK_PHYSICAL_DEVICE_TYPE_##r: \
        return #r
        STR(OTHER);
        STR(INTEGRATED_GPU);
        STR(DISCRETE_GPU);
        STR(VIRTUAL_GPU);
#undef STR
        default:
            return "UNKNOWN_DEVICE_TYPE";
    }
}

VkResult createInstance(VkInstance& instance, const char* appName) {
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = appName;
    appInfo.pEngineName = appName;
    appInfo.apiVersion = VK_API_VERSION_1_0;
    appInfo.applicationVersion = 1;
    appInfo.engineVersion = 1;

    std::vector<const char*> instanceExtensions = {
        VK_KHR_SURFACE_EXTENSION_NAME};
    // Enable surface extensions depending on os
#if defined(_WIN32)
    instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    instanceExtensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(_DIRECT2DISPLAY)
    instanceExtensions.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    instanceExtensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    instanceExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_IOS_MVK)
    instanceExtensions.push_back(VK_MVK_IOS_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
    instanceExtensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#endif

    VkInstanceCreateInfo instInfo = {};
    instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instInfo.pNext = NULL;
    instInfo.flags = 0;
    instInfo.pApplicationInfo = &appInfo;
    instInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
    instInfo.ppEnabledExtensionNames = instanceExtensions.data();
    // validation
    return vkCreateInstance(&instInfo, nullptr, &instance);
}

VkResult enumerateDevice(VkInstance instance,
                         std::vector<vkx::PhysicalDevice>& pDevices) {
    VkResult err;
    // Physical device
    uint32_t gpuCount = 0;
    // Get number of available physical devices
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr));
    assert(gpuCount > 0);
    // Enumerate devices
    pDevices.resize(gpuCount);
    std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
    err =
        vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices.data());
    for (uint32_t i = 0; i < gpuCount; i++) {
        pDevices[i].physicalDevice = physicalDevices[i];
    }
    if (err != VK_SUCCESS) {
        return err;
    }
    for (uint32_t i = 0; i < gpuCount; i++) {
        vkGetPhysicalDeviceProperties(pDevices[i].physicalDevice,
                                      &pDevices[i].properties);
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(pDevices[i].physicalDevice,
                                                 &queueFamilyCount, nullptr);
        if (queueFamilyCount > 0) {
            pDevices[i].queueFamilyProps.resize(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(
                pDevices[i].physicalDevice, &queueFamilyCount,
                pDevices[i].queueFamilyProps.data());
            vkGetPhysicalDeviceMemoryProperties(pDevices[i].physicalDevice,
                                                &pDevices[i].mempryProperties);
            vkGetPhysicalDeviceProperties(pDevices[i].physicalDevice,
                                          &pDevices[i].properties);
        }
        for (int j = 0; j < pDevices[i].queueFamilyProps.size(); j++) {
            if ((pDevices[i].queueFamilyProps[j].queueFlags &
                 VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT) {
                pDevices[i].queueGraphicsIndexs.push_back(j);
            }
            if ((pDevices[i].queueFamilyProps[j].queueFlags &
                 VK_QUEUE_COMPUTE_BIT) == VK_QUEUE_COMPUTE_BIT) {
                pDevices[i].queueComputeIndexs.push_back(j);
            }
        }
    }
    return VK_SUCCESS;
}

VkResult createLogicalDevice(vkx::LogicalDevice& device,
                             const vkx::PhysicalDevice& physicalDevice,
                             uint32_t queueFamilyIndex, bool bAloneCompute) {
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    float queuePriorities[1] = {0.0};
    VkDeviceQueueCreateInfo queueInfo = {};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = queueFamilyIndex;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = queuePriorities;
    queueCreateInfos.push_back(queueInfo);
    device.graphicsIndex = queueInfo.queueFamilyIndex;
    device.computeIndex = device.graphicsIndex;
    if (bAloneCompute) {
        // 找一个在通道只有CS,没有GS
        uint32_t index = queueFamilyIndex;
        for (auto cindex : physicalDevice.queueComputeIndexs) {
            if (cindex != index) {
                index = cindex;
                break;
            }
        }
        if (index != queueFamilyIndex) {
            queueInfo = {};
            queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.queueFamilyIndex = index;
            queueInfo.queueCount = 1;
            queueInfo.pQueuePriorities = queuePriorities;
            queueCreateInfos.push_back(queueInfo);
        }
        device.computeIndex = index;
    }
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount =
        static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.pEnabledFeatures = nullptr;
    std::vector<const char*> deviceExtensions;
    deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
    return vkCreateDevice(physicalDevice.physicalDevice, &deviceCreateInfo,
                          nullptr, &device.device);
}
int32_t getByteSize(VkFormat format) {
    switch (format) {
        case VK_FORMAT_R8G8B8A8_UNORM:
            return 4;
            break;
        default:
            break;
    }
    return 4;
}

bool getMemoryTypeIndex(const vkx::PhysicalDevice& physicalDevice,
                        uint32_t typeBits, VkFlags quirementsMaks,
                        uint32_t& index) {
    for (uint32_t i = 0; i < physicalDevice.mempryProperties.memoryTypeCount;
         i++) {
        if ((typeBits & 1) == 1) {
            // Type is available, does it match user properties?
            if ((physicalDevice.mempryProperties.memoryTypes[i].propertyFlags &
                 quirementsMaks) == quirementsMaks) {
                index = i;
                return true;
            }
        }
        typeBits >>= 1;
    }
    return false;
}
}  // namespace common
}  // namespace vkx