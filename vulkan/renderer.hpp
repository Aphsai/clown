#pragma once 
#include "platform.hpp"
#include <vector>
#include <string>

class Window;

class Renderer {
    public:
        Renderer();
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

        Window* openWindow(uint32_t size_x, uint32_t size_y, std::string name);

        VkInstance instance = VK_NULL_HANDLE;
        VkPhysicalDevice gpu = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        VkQueue queue = VK_NULL_HANDLE;
        uint32_t graphics_family_index = 0;
        VkPhysicalDeviceProperties gpu_properties = {};
        VkPhysicalDeviceMemoryProperties gpu_memory_properties = {};

        Window* window = nullptr;

        std::vector<const char*> instance_layers;
        std::vector<const char*> instance_extensions;
        std::vector<const char*> device_extensions;

        VkDebugReportCallbackEXT debug_report = VK_NULL_HANDLE;
        VkDebugReportCallbackCreateInfoEXT debug_callback_create_info = {};

};
