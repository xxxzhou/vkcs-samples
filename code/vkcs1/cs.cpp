#include <util_init.hpp>

int main(int argc, char *argv[]) {
    sample_info info = {};
    char sample_title[] = "Compute shader Sample";
    bool depthPresent = false;

    init_instance_extension_names(info);
    init_device_extension_names(info);
    init_instance(info, sample_title);
    init_enumerate_device(info);
    init_window_size(info, 500, 500);
    init_connection(info);
    init_window(info);
    init_swapchain_extension(info);
    init_device(info);
    init_command_pool(info);
    init_command_buffer(info);
    execute_begin_command_buffer(info);
    init_device_queue(info);
    init_swap_chain(info);
    init_depth_buffer(info);
    init_uniform_buffer(info);
    init_renderpass(info, depthPresent);
    init_framebuffers(info, depthPresent);

    

    return 0;
}