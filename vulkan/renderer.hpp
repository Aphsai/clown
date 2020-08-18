#pragma once 
#include "platform.hpp"
#include "vk_mem_alloc.h"
#include "window.hpp"
#include <vector>
#include <string>

struct Vertex;

class Renderer {
    public:
        Renderer (uint32_t size_x, uint32_t size_y, std::string name);
        ~Renderer();
        bool run();
        void setupLayersAndExtensions();

        void initInstance();
        void destroyInstance();
        void initDevice();
        void destroyDevice();
        void setupDebug();
        void initDebug();
        void destroyDebug();
        void initSurface();
        void destroySurface();
        

        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkInstance instance = VK_NULL_HANDLE;
        VkPhysicalDevice gpu = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        VkQueue graphics_queue = VK_NULL_HANDLE;
        VkQueue present_queue = VK_NULL_HANDLE;

        VkImageLayout image_layout = {};
        VkPhysicalDeviceProperties gpu_properties = {};
        VkPhysicalDeviceMemoryProperties gpu_memory_properties = {};
        VkSurfaceFormatKHR surface_format = {};
        VkSurfaceCapabilitiesKHR surface_capabilities = {};
        VkExtent2D swapchain_extent = {};

        uint32_t graphics_family_index = -1;
        uint32_t present_family_index = -1;
        uint32_t swapchain_image_count = 2;
        uint32_t active_swapchain_image_id = UINT32_MAX;
        uint32_t surface_size_x = 512;
        uint32_t surface_size_y = 512;
        uint32_t current_frame = 0;
        bool stencil_available = false;

        Window* window = nullptr;

        std::vector<const char*> instance_layers;
        std::vector<const char*> instance_extensions;
        std::vector<const char*> device_extensions;

        // DEBUG
        VkDebugReportCallbackEXT debug_report = VK_NULL_HANDLE;
        VkDebugReportCallbackCreateInfoEXT debug_callback_create_info = {};
};
