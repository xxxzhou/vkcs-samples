#include "VulkanBuffer.hpp"

#include "VulkanContext.hpp"
#include "VulkanHelper.hpp"
namespace vkx {
namespace common {
VulkanBuffer::VulkanBuffer() {}

VulkanBuffer::~VulkanBuffer() {
    if (view) {
        vkDestroyBufferView(device, view, nullptr);
    }
    if (buffer) {
        vkDestroyBuffer(device, buffer, nullptr);
    }
    if (memory) {
        vkFreeMemory(device, memory, nullptr);
    }
}

void VulkanBuffer::InitResource(class VulkanContext* context, uint32_t dataSize,
                                VkFormat viewFormat,
                                VkBufferUsageFlagBits usageFlag,
                                VkMemoryPropertyFlagBits memoryFlag,
                                uint8_t* cpuData) {
    this->device = context->logicalDevice.device;
    VkBufferCreateInfo bufInfo = {};
    bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufInfo.pNext = nullptr;
    bufInfo.usage = usageFlag;  //用来指定用于VBO/IBO/UBO等
    bufInfo.size = dataSize;
    bufInfo.queueFamilyIndexCount = 0;
    bufInfo.pQueueFamilyIndices = nullptr;
    bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufInfo.flags = 0;
    // 生成一个buffer标识(可以多个标识到同一个显存空间DeviceMemory里,标识用来指定VBO/IBO/UBO等)
    VK_CHECK_RESULT(vkCreateBuffer(device, &bufInfo, nullptr, &buffer));
    VkMemoryRequirements requires;
    vkGetBufferMemoryRequirements(device, buffer, &requires);
    uint32_t memoryTypeIndex = 0;
    bool getIndex =
        getMemoryTypeIndex(context->physicalDevice, requires.memoryTypeBits,
                           memoryFlag, memoryTypeIndex);
    assert(getIndex == true);
    VkMemoryAllocateInfo memoryInfo = {};
    memoryInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryInfo.pNext = nullptr;
    memoryInfo.memoryTypeIndex = memoryTypeIndex;
    memoryInfo.allocationSize = requires.size;
    // 生成设备显存空间
    VK_CHECK_RESULT(vkAllocateMemory(device, &memoryInfo, nullptr, &memory));
    if (cpuData) {
        uint8_t* pData = nullptr;
        VK_CHECK_RESULT(
            vkMapMemory(device, memory, 0, dataSize, 0, (void**)&pData));
        memcpy(pData, cpuData, dataSize);
        vkUnmapMemory(device, memory);
    }
    VK_CHECK_RESULT(vkBindBufferMemory(device, buffer, memory, 0));

    VkBufferViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    viewInfo.pNext = nullptr;
    viewInfo.buffer = buffer;
    viewInfo.format = viewFormat;  // VK_FORMAT_R32_SFLOAT;
    viewInfo.offset = 0;
    viewInfo.range = requires.size;
    VK_CHECK_RESULT(vkCreateBufferView(device, &viewInfo, nullptr, &view));

    info.buffer = buffer;
    info.offset = 0;
    info.range = dataSize;
}
}  // namespace common
}  // namespace vkx