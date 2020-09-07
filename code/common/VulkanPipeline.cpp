#include "VulkanPipeline.hpp"

#include <algorithm>

#include "VulkanContext.hpp"
namespace vkx {
namespace common {

int32_t UBOLayout::AddItem(VkDescriptorType type, VkShaderStageFlags stage,
                           uint32_t groupIndex) {
    if (groupIndex >= groups.size()) {
        groups.resize(groupIndex + 1);
        groupSize.resize(groupIndex + 1);
        groupSize[groupIndex] = 1;
    }
    UBOLayoutItem item = {};
    item.descriptorType = type;
    item.groupIndex = groupIndex;
    item.shaderStageFlags = stage;
    items.push_back(item);
    int32_t index = (int32_t)items.size() - 1;
    groups[groupIndex].push_back(index);
    return index;
}

void UBOLayout::SetGroupCount(uint32_t groupIndex, uint32_t count) {
    assert(groupIndex < groupSize.size());
    groupSize[groupIndex] = count;
}

void UBOLayout::GenerateLayout() {
    // group
    groupCount = 0;
    for (auto i = 0; i < groups.size(); i++) {
        if (groups[i].size() > 0) {
            int size = groupSize[i];
            if (size < 1) {
                size = 1;
            }
            groupCount += size;
        }
    }
    auto size = items.size();
    for (auto i = 0; i < size; i++) {
        int size = groupSize[items[i].groupIndex];
        if (size < 1) {
            size = 1;
        }
        // 可以假设map健值初始化为0是安全的 std::map<int,int> x;x[1]++;
        // https://stackoverflow.com/questions/2667355/mapint-int-default-values
        descripts[items[i].descriptorType] += size;
    }
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (const auto& kv : descripts) {
        if (kv.second > 0) {
            VkDescriptorPoolSize descriptorPoolSize = {};
            descriptorPoolSize.type = kv.first;
            descriptorPoolSize.descriptorCount = kv.second;
            poolSizes.push_back(descriptorPoolSize);
        }
    }
    // 创建VkDescriptorPool
    VkDescriptorPoolCreateInfo descriptorPoolInfo{};
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.poolSizeCount = (uint32_t)poolSizes.size();
    descriptorPoolInfo.pPoolSizes = poolSizes.data();
    descriptorPoolInfo.maxSets = (uint32_t)groupCount;
    VK_CHECK_RESULT(vkCreateDescriptorPool(context->logicalDevice.device,
                                           &descriptorPoolInfo, nullptr,
                                           &descPool));
    descSetLayouts.resize(groups.size());
    // 创建VkDescriptorSetLayout
    for (auto x = 0; x < groups.size(); x++) {
        auto& group = groups[x];
        int j = 0;
        std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
        for (auto i = 0; i < group.size(); i++) {
            // item 对应的字段
            VkDescriptorSetLayoutBinding setLayoutBinding = {};
            setLayoutBinding.descriptorType = items[i].descriptorType;
            setLayoutBinding.stageFlags = items[i].descriptorType;
            setLayoutBinding.binding = j++;
            setLayoutBinding.descriptorCount = 1;
            layoutBindings.push_back(setLayoutBinding);
        }
        VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo = {};
        descriptorLayoutInfo.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorLayoutInfo.pBindings = layoutBindings.data();
        descriptorLayoutInfo.bindingCount = layoutBindings.size();
        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(
            context->logicalDevice.device, &descriptorLayoutInfo, nullptr,
            &descSetLayouts[x]));
    }
}  // namespace common

VulkanPipeline::VulkanPipeline(/* args */) {}

VulkanPipeline::~VulkanPipeline() {}

void VulkanPipeline::CreateDefaultFixPipelineState(FixPipelineState& fix) {
    // ---图元装配
    fix.inputAssemblyState.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    // 这个flags是啥?使用场景末知
    fix.inputAssemblyState.flags = 0;
    // mesh常用
    fix.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    fix.inputAssemblyState.primitiveRestartEnable = false;
    // ---光栅化
    fix.rasterizationState.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    fix.rasterizationState.flags = 0;
    // 默认情况,填充面
    fix.rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    // 背面剔除
    fix.rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
    // 指定顺时针为正面
    fix.rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
    // 阴影贴图启用这个选项
    fix.rasterizationState.depthBiasEnable = VK_FALSE;
    fix.rasterizationState.depthBiasConstantFactor = 0.0f;  // Optional
    fix.rasterizationState.depthBiasClamp = 0.0f;           // Optional
    fix.rasterizationState.depthBiasSlopeFactor = 0.0f;     // Optional
    // ---片版与老的桢缓冲像素混合
    fix.coolrAttach.colorWriteMask = 0xf;
    fix.coolrAttach.blendEnable = VK_FALSE;
    fix.colorBlendState.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    fix.colorBlendState.attachmentCount = 1;
    fix.colorBlendState.pAttachments = &fix.coolrAttach;
    // ---深度和模板测试
    VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
    fix.depthStencilState.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    fix.depthStencilState.depthTestEnable = VK_TRUE;
    fix.depthStencilState.depthWriteEnable = VK_TRUE;
    // 新深度小于或等于老深度通过,画家算法
    fix.depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    fix.depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
    // ---视口和裁剪矩形
    VkPipelineViewportStateCreateInfo viewportState = {};
    fix.viewportState.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    fix.viewportState.viewportCount = 0;
    fix.viewportState.scissorCount = 0;
    fix.viewportState.flags = 0;
    // ---多重采集(可以在渲染GBUFFER关闭,渲染最终颜色时打开)
    VkPipelineMultisampleStateCreateInfo multisampleState = {};
    fix.multisampleState.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    fix.multisampleState.flags = 0;
    fix.multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    // ---管线动态修改
    fix.dynamicStateEnables.clear();
    fix.dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    fix.dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);
    fix.dynamicState.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    fix.dynamicState.flags = 0;
    fix.dynamicState.pDynamicStates = fix.dynamicStateEnables.data();
    fix.dynamicState.dynamicStateCount = fix.dynamicStateEnables.size();
}

}  // namespace common
}  // namespace vkx
