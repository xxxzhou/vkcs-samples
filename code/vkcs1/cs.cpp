#include <VulkanContext.hpp>
#include <VulkanPipeline.hpp>
#include <VulkanWindow.hpp>
// int main(int argc, char *argv[]) {
//     char sample_title[] = "Compute shader Sample";

//     return 0;
// }
using namespace vkx::common;

void onDraw(uint32_t index) {}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    const char* title = "vkcs1";
    VulkanContext context = {};
    context.CreateInstance(title);
    VulkanWindow window(&context);
    window.InitWindow(hInstance, 1280, 720, title);
    context.CreateDevice(window.graphicsQueueIndex);

    FixPipelineState fixState = {};
    VulkanPipeline::CreateDefaultFixPipelineState(fixState);

    window.CreateSwipChain(context.logicalDevice.device, onDraw);
    window.Run();
}