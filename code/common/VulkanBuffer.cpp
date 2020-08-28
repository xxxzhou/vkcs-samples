#include "VulkanBuffer.hpp"

#include "VulkanHelper.hpp"

namespace vkx {
namespace common {
VulkanBuffer::VulkanBuffer(/* args */) {}

VulkanBuffer::~VulkanBuffer() {}

void VulkanBuffer::InitResource(VkDevice device, uint32_t dataSize,
                                VkBufferUsageFlagBits usageFlag,
                                uint32_t memoryTypeIndex, uint8_t *cpuData) {
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

    VkMemoryAllocateInfo memoryInfo = {};
    memoryInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryInfo.pNext = nullptr;
    memoryInfo.memoryTypeIndex = memoryTypeIndex;
    memoryInfo.allocationSize = requires.size;
    // 生成设备显存空间
    VK_CHECK_RESULT(vkAllocateMemory(device, &memoryInfo, nullptr, &memory));
    if (cpuData) {
        uint8_t *pData = nullptr;
        VK_CHECK_RESULT(
            vkMapMemory(device, memory, 0, dataSize, 0, (void **)&pData));
        memcpy(pData, cpuData, dataSize);
        vkUnmapMemory(device, memory);
    }
    VK_CHECK_RESULT(vkBindBufferMemory(device, buffer, memory, 0));
}
}  // namespace common
}  // namespace vkx