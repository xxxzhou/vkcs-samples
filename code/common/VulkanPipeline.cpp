#include "VulkanPipeline.hpp"

#include <algorithm>
#include <cstdarg>

#include "VulkanContext.hpp"
namespace vkx {
namespace common {

UBOLayout::UBOLayout(class VulkanContext* _context) {
    this->context = _context;
}

UBOLayout::~UBOLayout() {
    VkDevice device = context->logicalDevice.device;
    if (device) {
        auto size = items.size();
        for (auto i = 0; i < size; i++) {
            vkDestroyDescriptorSetLayout(device, descSetLayouts[i], nullptr);
            items[i].clear();
        }
        if (descPool) {
            vkDestroyDescriptorPool(device, descPool, nullptr);
        }
        if (pipelineLayout) {
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        }
        // vkFreeDescriptorSets
        items.clear();
        descripts.clear();
    }
}

int32_t UBOLayout::AddSetLayout(std::vector<UBOLayoutItem> layout,
                                uint32_t count) {
    items.push_back(layout);
    groupSize.push_back(count > 1 ? count : 1);
    return items.size() - 1;
}

void UBOLayout::GenerateLayout() {
    // 最多需要多少个set,一个layout可能有多个set,比如渲染8个相同物体
    uint32_t groupCount = 0;
    auto size = items.size();
    for (auto i = 0; i < size; i++) {
        groupCount += groupSize[i];
        for (auto j = 0; j < items[i].size(); j++) {
            // 可以假设map健值初始化为0是安全的 std::map<int,int> x;x[1]++;
            // https://stackoverflow.com/questions/2667355/mapint-int-default-values
            descripts[items[i][j].descriptorType] += groupSize[i];
        }
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
    descriptorPoolInfo.maxSets = groupCount;
    VK_CHECK_RESULT(vkCreateDescriptorPool(context->logicalDevice.device,
                                           &descriptorPoolInfo, nullptr,
                                           &descPool));
    descSetLayouts.resize(size);
    descSets.resize(size);
    // 创建VkDescriptorSetLayout
    for (auto x = 0; x < size; x++) {
        auto& group = items[x];
        if (group.size() < 1) {
            continue;
        }
        std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
        for (auto i = 0; i < group.size(); i++) {
            // item 对应的字段
            VkDescriptorSetLayoutBinding setLayoutBinding = {};
            setLayoutBinding.descriptorType = group[i].descriptorType;
            setLayoutBinding.stageFlags = group[i].shaderStageFlags;
            setLayoutBinding.binding = i;
            setLayoutBinding.descriptorCount = 1;
            layoutBindings.push_back(setLayoutBinding);
        }
        VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo = {};
        descriptorLayoutInfo.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorLayoutInfo.pBindings = layoutBindings.data();
        descriptorLayoutInfo.bindingCount = (uint32_t)layoutBindings.size();
        // 生成VkDescriptorSetLayout
        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(
            context->logicalDevice.device, &descriptorLayoutInfo, nullptr,
            &descSetLayouts[x]));
        // 生成VkDescriptorSet
        descSets[x].resize(groupSize[x]);
        VkDescriptorSetAllocateInfo descAllocInfo = {};
        descAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descAllocInfo.descriptorPool = descPool;
        descAllocInfo.pSetLayouts = &descSetLayouts[x];
        descAllocInfo.descriptorSetCount = groupSize[x];
        VK_CHECK_RESULT(vkAllocateDescriptorSets(
            context->logicalDevice.device, &descAllocInfo, descSets[x].data()));
    }
    // 生成pipelineLayout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = descSetLayouts.size();
    pipelineLayoutCreateInfo.pSetLayouts = descSetLayouts.data();
    VK_CHECK_RESULT(vkCreatePipelineLayout(context->logicalDevice.device,
                                           &pipelineLayoutCreateInfo, nullptr,
                                           &pipelineLayout));
}

void UBOLayout::UpdateSetLayout(uint32_t groupIndex, uint32_t setIndex, ...) {
    assert(groupIndex >= 0 && groupIndex < items.size());
    va_list args;
    va_start(args, setIndex);
    auto& descSet = descSets[groupIndex][setIndex];
    auto& group = items[groupIndex];
    std::vector<VkWriteDescriptorSet> writes;
    for (auto i = 0; i < group.size(); i++) {
        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = descSet;
        write.dstBinding = i;
        write.descriptorType = group[i].descriptorType;
        switch (write.descriptorType) {
            // pImageInfo里的sample
            case VK_DESCRIPTOR_TYPE_SAMPLER:
            // pImageInfo里的imageView与imageLayout
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            // pImageInfo里的所有成员
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                write.pImageInfo = va_arg(args, const VkDescriptorImageInfo*);
                break;
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                write.pBufferInfo = va_arg(args, const VkDescriptorBufferInfo*);
                break;
            case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                write.pTexelBufferView = va_arg(args, const VkBufferView*);
            default:
                break;
        }
        write.descriptorCount = 1;
        writes.push_back(write);
    }
    va_end(args);
    vkUpdateDescriptorSets(context->logicalDevice.device,
                           static_cast<uint32_t>(writes.size()), writes.data(),
                           0, nullptr);
}

// for (auto i = 0; i < group.size(); i++) {
//     VkWriteDescriptorSet writeDesc = {};
//     writeDesc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//     writeDesc.dstSet = descSets[x][i];
//     writeDesc.descriptorType = type;
//     writeDesc.dstBinding = binding;
//     writeDesc.pBufferInfo = bufferInfo;
//     writeDesc.descriptorCount = descriptorCount; /* code */
// }

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
    fix.dynamicState.dynamicStateCount =
        (uint32_t)fix.dynamicStateEnables.size();
}
VkPipelineShaderStageCreateInfo VulkanPipeline::LoadShader(
#if __ANDROID__
    AAssetManager* assetManager,
#endif
    VkDevice device, const std::string& fileName, VkShaderStageFlagBits stage) {
    VkPipelineShaderStageCreateInfo shaderStage = {};
    shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage.stage = stage;
#if defined(__ANDROID__)
    shaderStage.module = loadShader(assetManager,
                                    fileName.c_str(), device);
#else
    shaderStage.module = loadShader(fileName.c_str(), device);
#endif
    shaderStage.pName = "main";
    return shaderStage;
}
VkComputePipelineCreateInfo VulkanPipeline::CreateComputePipelineInfo(
    VkPipelineLayout layout, VkPipelineShaderStageCreateInfo stageInfo) {
    VkComputePipelineCreateInfo computePipelineInfo = {};
    computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineInfo.layout = layout;
    computePipelineInfo.flags = 0;
    computePipelineInfo.stage = stageInfo;
    return computePipelineInfo;
}

}  // namespace common
}  // namespace vkx
