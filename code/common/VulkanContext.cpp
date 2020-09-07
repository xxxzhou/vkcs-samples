#include "VulkanContext.hpp"
namespace vkx {
namespace common {
VulkanContext::VulkanContext(/* args */) {}

VulkanContext::~VulkanContext() {}

void VulkanContext::CreateInstance(const char* appName) {
    createInstance(instace, appName);
    std::vector<vkx::PhysicalDevice> physicalDevices;
    enumerateDevice(instace, physicalDevices);
    // 选择第一个物理设备
    this->physicalDevice = physicalDevices[0];
}

void VulkanContext::CreateDevice(uint32_t graphicsIndex, bool bAloneCompute) {
    // 创建虚拟设备
    createLogicalDevice(logicalDevice, physicalDevice, graphicsIndex,
                        bAloneCompute);
    vkGetDeviceQueue(logicalDevice.device, logicalDevice.computeIndex, 0,
                     &computeQueue);
    vkGetDeviceQueue(logicalDevice.device, logicalDevice.graphicsIndex, 0,
                     &graphicsQueue);
}
}  // namespace common
}  // namespace vkx