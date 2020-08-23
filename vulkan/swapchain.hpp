#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <set>
#include <algorithm>

class Swapchain {
    public:
        Swapchain();
        ~Swapchain();

        static VkSurfaceFormatKHR chooseSwapchainFormat(const std::vector<VkSurfaceFormatKHR> &available_formats);
        static VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR> &available_present_modes);
        static VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR &capabilities);
};
