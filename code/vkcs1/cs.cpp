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
int32_t width = 1920;
int32_t height = 1080;
VulkanContext context = {};
std::unique_ptr<VulkanWindow> window = nullptr;
FixPipelineState fixState = {};
std::unique_ptr<UBOLayout> layout = nullptr;
std::unique_ptr<VulkanTexture> csSrcTex = nullptr;
std::unique_ptr<VulkanBuffer> srcBuffer = nullptr;
bool bSupport = false;
void onDraw(uint32_t index) {
    VkImage winImage = window->images[index];
    VkCommandBuffer cmd = window->cmdBuffers[index];
    // 我们要把cs生成的图复制到正在渲染的图上,先改变渲染图的layout
    changeLayout(cmd, winImage, VK_IMAGE_LAYOUT_UNDEFINED,
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

    if (bSupport) {
        // csSrcTex做为复制源准备
        csSrcTex->ChangeLayout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               VK_PIPELINE_STAGE_TRANSFER_BIT);
        context.BlitFillImage(cmd, csSrcTex.get(), winImage, window->width,
                              window->height);
    } else {
        // csSrcTex做为被复制准备
        csSrcTex->ChangeLayout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               VK_PIPELINE_STAGE_TRANSFER_BIT);
        // buffer复制到tex
        context.BufferToImage(cmd, srcBuffer.get(), csSrcTex.get());
        // csSrcTex做为复制源准备
        csSrcTex->ChangeLayout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               VK_PIPELINE_STAGE_TRANSFER_BIT);
        context.BlitFillImage(cmd, csSrcTex.get(), winImage, window->width,
                              window->height);
    }
    // 复制完成后,改变渲染图的layout准备呈现
    changeLayout(
        cmd, winImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    const char* title = "vkcs1";
    VulkanContext context = {};
    context.CreateInstance(title);
    window = std::make_unique<VulkanWindow>(&context);
    window->InitWindow(hInstance, 1280, 720, title);
    context.CreateDevice(window->graphicsQueueIndex);

    VulkanPipeline::CreateDefaultFixPipelineState(fixState);

    layout = std::make_unique<UBOLayout>(&context);
    layout->AddSetLayout(
        {{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT},
         {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT}});
    layout->GenerateLayout();

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
    // 一般是不能把CPU数据直接给性线storage.
    // VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT
    bSupport = context.CheckFormat(VK_FORMAT_B8G8R8A8_UNORM,
                                   VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT |
                                       VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT,
                                   true);

    csSrcTex = std::make_unique<VulkanTexture>();
    srcBuffer = std::make_unique<VulkanBuffer>();
    if (bSupport) {
        csSrcTex->InitResource(
            &context, width, height, VkFormat::VK_FORMAT_B8G8R8A8_UNORM,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            vdata.data());
    } else {
        // VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        srcBuffer->InitResource(&context, size, VK_FORMAT_R8_UNORM,
                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                vdata.data());
        csSrcTex->InitResource(&context, width, height,
                               VkFormat::VK_FORMAT_B8G8R8A8_UNORM,
                               VK_IMAGE_USAGE_TRANSFER_DST_BIT, 0);
    }

    const std::string path = getAssetPath() + "glsl/cs1.comp.spv";
    auto shaderInfo = VulkanPipeline::LoadShader(
        context.logicalDevice.device, path, VK_SHADER_STAGE_COMPUTE_BIT);
    auto computePipelineInfo = VulkanPipeline::CreateComputePipelineInfo(
        layout->pipelineLayout, shaderInfo);

    window->CreateSwipChain(context.logicalDevice.device, onDraw);
    window->Run();
}