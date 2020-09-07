#pragma once
// 从渲染来说,管线大致有几种,透明物体/非透明物体/UI/后处理特效,纯计算
// 管线需要设置的值大致对应shader相关
// 从UBO,Texture
#include <map>

#include "VulkanCommon.hpp"
namespace vkx {
namespace common {

template class VKX_COMMON_EXPORT std::vector<VkDynamicState>;

// 可以由VulkanPipeline创建一个默认填充的FixPipelineState.
// 根据渲染非透明/透明/后处理/GUBFFER/阴影 不同条件修改FixPipelineState
struct FixPipelineState {
   public:
    // ---图元装配
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
    // ---光栅化
    VkPipelineRasterizationStateCreateInfo rasterizationState = {};
    // ---片版与老的桢缓冲像素混合
    VkPipelineColorBlendAttachmentState coolrAttach = {};
    VkPipelineColorBlendStateCreateInfo colorBlendState = {};
    // ---深度和模板测试
    VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
    // ---视口和裁剪矩形
    VkPipelineViewportStateCreateInfo viewportState = {};
    // ---多重采集(可以在渲染GBUFFER关闭,渲染最终颜色时打开)
    VkPipelineMultisampleStateCreateInfo multisampleState = {};
    // ---管线动态修改
    std::vector<VkDynamicState> dynamicStateEnables = {};
    VkPipelineDynamicStateCreateInfo dynamicState = {};
};

class UBOLayoutItem {
   public:
    VkDescriptorType descriptorType;
    // 可以组合,简单来说,UBO可以绑定到几个shader阶段
    VkShaderStageFlags shaderStageFlags;
    // 设计不同group,对应不同set
    uint32_t groupIndex = 0;
};

// 对应shader里固定结构的结构以及数据
class UBOLayout {
   private:
    class VulkanContext* context;
    std::vector<UBOLayoutItem> items;

   private:
    std::map<VkDescriptorType, uint32_t> descripts;
    std::vector<std::vector<int>> groups = {};
    std::vector<int> groupSize = {1};
    uint32_t groupCount = 1;
    // 确定所有UBO不同descriptorType的总结
    VkDescriptorPool descPool;
    // 根据groupIndex分组生成不同VkDescriptorSetLayout
    std::vector<VkDescriptorSetLayout> descSetLayouts;
    // 根据layout生成不同的
    std::vector<VkDescriptorSet> descSets;

   public:
    VkPipelineLayout pipelineLayout = nullptr;

   public:
    // 需要注意,这个AddItem顺序很重要,对照shader上的顺序来,返回当前groud里的index
    int32_t AddItem(VkDescriptorType type, VkShaderStageFlags stage,
                    uint32_t groupIndex = 0);
    // 一个group有几个set
    void SetGroupCount(uint32_t groupIndex, uint32_t count = 1);
    void GenerateLayout();

    // void UpdateItem(uint32_t index, )
};

class VKX_COMMON_EXPORT VulkanPipeline {
   private:
    /* data */
   public:
    VulkanPipeline(/* args */);
    ~VulkanPipeline();

   public:
   public:
    // 创建一个默认状态的管线
    void static CreateDefaultFixPipelineState(FixPipelineState& fixState);
};
}  // namespace common
}  // namespace vkx