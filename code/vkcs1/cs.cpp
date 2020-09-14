#include <VulkanContext.hpp>
#include <VulkanPipeline.hpp>
#include <VulkanWindow.hpp>
#include <memory>
#include <string>

#include "cs1.comp.h"
// int main(int argc, char *argv[]) {
//     char sample_title[] = "Compute shader Sample";

//     return 0;
// }

using namespace vkx::common;

// 生成一张图
static int32_t width = 1920;
static int32_t height = 1080;
static std::unique_ptr<VulkanContext> context = {};
static std::unique_ptr<VulkanWindow> window = nullptr;
static FixPipelineState fixState = {};
static std::unique_ptr<UBOLayout> layout = nullptr;
static std::unique_ptr<VulkanTexture> csDestTex = nullptr;
static std::unique_ptr<VulkanTexture> csSrcTex = nullptr;
static std::unique_ptr<VulkanBuffer> srcBuffer = nullptr;
static VkFence computerFence;
static VkPipeline computerPipeline;
static bool bSupport = false;

void computeCommand() {
    // 创建cpu-gpu通知
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // 默认是有信号
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(context->logicalDevice.device, &fenceInfo, nullptr,
                  &computerFence);
    VkCommandBuffer cmd = context->computerCmd;
    VkCommandBufferBeginInfo cmdBeginInfo = {};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CHECK_RESULT(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    // csSrcTex做为被复制准备
    csSrcTex->AddBarrier(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         VK_PIPELINE_STAGE_TRANSFER_BIT);
    // buffer复制到tex
    context->BufferToImage(cmd, srcBuffer.get(), csSrcTex.get());
    // csSrcTex为cs计算的源
    csSrcTex->AddBarrier(cmd, VK_IMAGE_LAYOUT_GENERAL,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    // csDestTex做为cs计算的目的
    csDestTex->AddBarrier(cmd, VK_IMAGE_LAYOUT_GENERAL,
                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                          VK_ACCESS_SHADER_WRITE_BIT);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computerPipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                            layout->pipelineLayout, 0, 1,
                            layout->descSets[0].data(), 0, 0);
    // 执行cs
    vkCmdDispatch(cmd, width / 32, height / 8, 1);
    // 准备复制给呈现列表
    csDestTex->AddBarrier(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                          VK_PIPELINE_STAGE_TRANSFER_BIT,
                          VK_ACCESS_SHADER_READ_BIT);
    // 记录状态改为可执行状态
    vkEndCommandBuffer(cmd);
}

void onPreDraw() {
    auto device = context->logicalDevice.device;
    vkWaitForFences(device, 1, &computerFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &computerFence);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &context->computerCmd;

    VK_CHECK_RESULT(
        vkQueueSubmit(context->computeQueue, 1, &submitInfo, computerFence));
}

void onPreCommand(uint32_t index) {
    VkImage winImage = window->images[index];
    VkCommandBuffer cmd = window->cmdBuffers[index];
    // 我们要把cs生成的图复制到正在渲染的图上,先改变渲染图的layout
    changeLayout(cmd, winImage, VK_IMAGE_LAYOUT_UNDEFINED,
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                 VK_PIPELINE_STAGE_TRANSFER_BIT);
    context->BlitFillImage(cmd, csDestTex.get(), winImage, window->width,
                           window->height);
    // 复制完成后,改变渲染图的layout准备呈现
    changeLayout(cmd, winImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                 VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                 VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    const char* title = "vkcs1";
    context = std::make_unique<VulkanContext>();
    context->CreateInstance(title);
    window = std::make_unique<VulkanWindow>(context.get());
    window->InitWindow(hInstance, 1280, 720, title);
    context->CreateDevice(window->graphicsQueueIndex);

    VulkanPipeline::CreateDefaultFixPipelineState(fixState);

    int32_t size = width * height * 4;
    std::vector<uint8_t> vdata(size, 0);
    uint8_t* data = vdata.data();
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            unsigned char rgb = (((row & 0x8) == 0) ^ ((col & 0x8) == 0)) * 255;
            data[0] = rgb;
            data[1] = rgb;
            data[2] = rgb;
            data[3] = 255;
            data += 4;
        }
    }
    // 先检查设备是否支持纹理线性sampled|storage.一般不会支持线性storage
    // 一般是不能把CPU数据直接给性线storage.VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT
    bSupport = context->CheckFormat(VK_FORMAT_B8G8R8A8_UNORM,
                                    VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT |
                                        VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT,
                                    true);

    csSrcTex = std::make_unique<VulkanTexture>();
    csDestTex = std::make_unique<VulkanTexture>();
    // 一般来说,用于VK_IMAGE_USAGE_STORAGE_BIT不支持line,也就是VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
    srcBuffer = std::make_unique<VulkanBuffer>();
    // VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    srcBuffer->InitResource(context.get(), size, VK_FORMAT_R8_UNORM,
                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            vdata.data());
    csSrcTex->InitResource(
        context.get(), width, height, VK_FORMAT_B8G8R8A8_UNORM,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0);

    csDestTex->InitResource(
        context.get(), width, height, VK_FORMAT_B8G8R8A8_UNORM,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, 0);

    layout = std::make_unique<UBOLayout>(context.get());
    // 在计算管线里,定义二个纹理分别用做输入输出
    layout->AddSetLayout(
        {{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT},
         {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT}});
    layout->GenerateLayout();
    // 二个纹理对应的实体
    layout->UpdateSetLayout(0, 0,
                            csSrcTex->GetDescInfo(VK_IMAGE_LAYOUT_GENERAL),
                            csDestTex->GetDescInfo(VK_IMAGE_LAYOUT_GENERAL));

    // getAssetPath() + "glsl/cs1.comp.spv"
    const std::string path =
        "D:/WorkSpace/github/vkcs-samples/code/data/glsl/cs1.comp.spv";

    auto shaderInfo = VulkanPipeline::LoadShader(
        context->logicalDevice.device, path, VK_SHADER_STAGE_COMPUTE_BIT);
    auto computePipelineInfo = VulkanPipeline::CreateComputePipelineInfo(
        layout->pipelineLayout, shaderInfo);
    VK_CHECK_RESULT(vkCreateComputePipelines(
        context->logicalDevice.device, context->pipelineCache, 1,
        &computePipelineInfo, nullptr, &computerPipeline));
    window->CreateSwipChain(context->logicalDevice.device, onPreCommand);
    computeCommand();
    window->Run(onPreDraw);  // onPreDraw
}