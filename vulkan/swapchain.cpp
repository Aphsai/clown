#include "swapchain.hpp"
#include "renderer.hpp"

VkSurfaceFormatKHR Swapchain::chooseSwapchainFormat(const std::vector<VkSurfaceFormatKHR> &available_formats) {
    if (available_formats.size() == 1 && available_formats[0].format == VK_FORMAT_UNDEFINED) {
        return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    }

    for (const auto &format : available_formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }

    return available_formats[0];
}
VkPresentModeKHR Swapchain::choosePresentMode(const std::vector<VkPresentModeKHR> &available_present_modes) {
    VkPresentModeKHR best_mode = VK_PRESENT_MODE_FIFO_KHR;

    for (const auto& available_mode : available_present_modes) {
        if (available_mode == VK_PRESENT_MODE_MAILBOX_KHR || available_mode == VK_PRESENT_MODE_FIFO_KHR) {
            return available_mode;
        }

        if (available_mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            best_mode = available_mode;
        }
    }

    return best_mode;
}
VkExtent2D Swapchain::chooseExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } 

    VkExtent2D actual_extent = { 1280, 720 };

    actual_extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actual_extent.width));
    actual_extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actual_extent.height));

    return actual_extent;
}
