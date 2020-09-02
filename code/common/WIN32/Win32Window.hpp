#pragma once
#include <windows.h>

namespace vkx {
namespace common {
class Win32Window {
   private:
    /* data */
   public:
    Win32Window(/* args */);
    ~Win32Window();

   public:
    HWND hwnd;

   public:
    HWND InitWindow(HINSTANCE inst, int width, int height, const char* name,
                    class VulkanSwapChain* swapChain);
};
}  // namespace common
}  // namespace vkx