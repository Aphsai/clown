#include "window.hpp"
#include "renderer.hpp"
#include "shared.hpp"
 
#include <assert.h>
#include <array>

Window::Window(Renderer* renderer, uint32_t size_x, uint32_t size_y, std::string name) {
    renderer = renderer;
    surface_size_x = size_x;
    surface_size_y = size_y;
    window_name = name;

    initOSWindow();
    initSurface();
    initSwapchain();
    initSwapchainImages();
    initDepthStencilImage();
    initRenderPass();
    initFramebuffers();
    initSynchronization();
}

Window::~Window() {
    vkQueueWaitIdle(renderer->queue);
    destroySynchronization();
    destroyFramebuffers();
    destroyRenderPass();
    destroyDepthStencilImage();
    destroySwapchainImages();
    destroySwapchain();
    destroySurface();
    destroyOSWindow();
}

void Window::close() {
    window_should_run = false;
}

bool Window::update() {
    updateOSWindow();
    return window_should_run;
}

void Window::beginRender() {
    errorCheck(vkAcquireNextImageKHR(
                renderer->device,
                swapchain,
                UINT64_MAX,
                VK_NULL_HANDLE,
                swapchain_image_available,
                &active_swapchain_image_id));
    errorCheck(vkWaitForFences(renderer->device, 1, &swapchain_image_available, VK_TRUE, UINT64_MAX));
    errorCheck(vkResetFences(renderer->device, 1, &swapchain_image_available));
    errorCheck(vkQueueWaitIdle(renderer->queue));
}

void Window::endRender(std::vector<VkSemaphore> wait_semaphores) {
    VkResult present_result = VkResult::VK_RESULT_MAX_ENUM;
    VkPresentInfoKHR present_info {};
    present_info.waitSemaphoreCount = wait_semaphores.size();
    present_info.pWaitSemaphores = wait_semaphores.data();
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &active_swapchain_image_id;
    present_info.pResults = &present_result;

    errorCheck(vkQueuePresentKHR(renderer->queue, &present_info));
    errorCheck(present_result);
}

void Window::initSurface() {
    initOSSurface();
    auto gpu = renderer->gpu;
    VkBool32 WSI_supported = false;
    errorCheck(vkGetPhysicalDeviceSurfaceSupportKHR(gpu, renderer->graphics_family_index, surface, &WSI_supported));
    if (!WSI_supported) {
        assert(0 && "WSI not supported");
        std::exit(-1);
    }

    errorCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &surface_capabilities));
    if (surface_capabilities.currentExtent.width < UINT32_MAX) {
        surface_size_x = surface_capabilities.currentExtent.width;
        surface_size_y = surface_capabilities.currentExtent.height;
    }

    uint32_t format_count = 0;
    errorCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &format_count, nullptr));
    if (format_count == 0) {
        assert(0 && "Surface formats missing");
        std::exit(-1);
    }
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    errorCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &format_count, formats.data()));
    if (formats[0].format == VK_FORMAT_UNDEFINED) {
        surface_format.format = VK_FORMAT_B8G8R8A8_UNORM;
        surface_format.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    } else {
        surface_format = formats[0];
    }
}

void Window::destroySurface() {
    vkDestroySurfaceKHR(renderer->instance, surface, nullptr);
}
