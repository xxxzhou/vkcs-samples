#include "VulkanContext.hpp"
namespace vkx {
namespace common {
VulkanContext::VulkanContext(/* args */) {}

VulkanContext::~VulkanContext() {}

void VulkanContext::CreateContext(const char* appName, uint32_t queueIndex) {
    createInstance(instace, appName);
    std::vector<vkx::PhysicalDevice> physicalDevices;
    enumerateDevice(instace, physicalDevices);
    // 选择第一个物理设备
    this->physicalDevice = physicalDevices[0];
    // 创建虚拟设备
    createLogicalDevice(logicalDevice, physicalDevice, queueIndex);
    //
}

}  // namespace common
}  // namespace vkx