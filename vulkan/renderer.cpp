#define VMA_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define BUILD_ENABLE_VULKAN_DEBUG 
#define BUILD_ENABLE_VULKAN_RUNTIME_DEBUG 

#include "stb_image.h"
#include "platform.hpp"
#include "renderer.hpp"
#include "shared.hpp"
#include "window.hpp"

#include <cstdlib>
#include <assert.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <set>


Renderer::Renderer(uint32_t size_x, uint32_t size_y, std::string name) {
    initPlatform();
    setupLayersAndExtensions();
    setupDebug();
    initInstance();
    initDebug();
    initWindow();
    initDevice();
    initSurface();
}

Renderer::~Renderer() {
}

void Renderer::initWindow() {
    window = new Window(this, size_x, size_y, name);
}

void Renderer::setupLayersAndExtensions() {
    instance_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    addRequiredPlatformInstanceExtensions(&instance_extensions);
    device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

void Renderer::initInstance() {
    VkApplicationInfo application_info {};
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.apiVersion = VK_MAKE_VERSION(1, 0, 0);
    application_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    application_info.pApplicationName = "Jester";

    VkInstanceCreateInfo instance_create_info = {};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo = &application_info;
    instance_create_info.enabledLayerCount = instance_layers.size();
    instance_create_info.ppEnabledLayerNames = instance_layers.data();
    instance_create_info.enabledExtensionCount = instance_extensions.size();
    instance_create_info.ppEnabledExtensionNames = instance_extensions.data();
    instance_create_info.pNext = &debug_callback_create_info;

    errorCheck(vkCreateInstance(&instance_create_info, nullptr, &instance));
}

void Renderer::destroyInstance() {
    vkDestroyInstance(instance, nullptr);
    instance = nullptr;
}

void Renderer::initDevice() {

    uint32_t gpu_count = 0;
    vkEnumeratePhysicalDevices(instance, &gpu_count, nullptr);
    std::vector<VkPhysicalDevice> gpu_list (gpu_count);
    vkEnumeratePhysicalDevices(instance, &gpu_count, gpu_list.data());

    assert(gpu_count > 0 && "VULKAN ERROR: Failed to find GPUs with Vulkan Support");

    bool found = false;

    for (const auto& device : gpu_list) {
        uint32_t family_count = 0;
        bool extensions_supported = false;

        vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, nullptr);
        std::vector<VkQueueFamilyProperties> family_property_list(family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, family_property_list.data());

        for (uint32_t x = 0; x < family_count; x++) {
            if (family_property_list[x].queueFlags & VK_QUEUE_GRAPHICS_BIT) graphics_family_index = x;
            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, x, surface, &present_support);
            if (present_support) present_family_index = x;
            if (present_family_index > 0 && graphics_family_index > 0) break;
        }

        uint32_t extension_count = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
        std::vector<VkExtensionProperties> available_extensions (extension_count);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

        std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());
        for (const auto& extension : available_extensions) required_extensions.erase(extension.extensionName);
        if (required_extensions.empty()) {
            VkSurfaceCapabilitiesKHR capabilities; 
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> present_modes;

            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);
            uint32_t format_count = 0;
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);

            if (format_count != 0) {
                formats.resize(format_count);
                vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, formats.data());
            }
            
            uint32_t present_mode_count = 0;
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);

            if (present_mode_count != 0) {
                present_modes.resize(present_mode_count);
                vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, present_modes.data());
            }

            if (!formats.empty() && !present_modes.empty()) {
                 VkPhysicalDeviceFeatures supported_features {};
                 vkGetPhysicalDeviceFeatures(device, &supported_features);
                 if (supported_features.samplerAnisotropy) {
                     gpu = device;
                     found = true;
                     break;
                 }
            }
        }

    }
    vkGetPhysicalDeviceProperties(gpu, &gpu_properties);
    vkGetPhysicalDeviceMemoryProperties(gpu, &gpu_memory_properties);

    if (!found) {
        assert(0 && "VULKAN ERROR: Queue family supporting graphics not found.");
        std::exit(-1);
    }

    std::vector<VkDeviceQueueCreateInfo> device_queue_create_infos;
    std::set<uint32_t> unique_queue_families = { graphics_family_index, present_family_index };

    float queue_priorities[] { 1.0f };
    
    VkDeviceQueueCreateInfo device_queue_create_info {};
    device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    device_queue_create_info.queueFamilyIndex = graphics_family_index;
    device_queue_create_info.queueCount = 1;
    device_queue_create_info.pQueuePriorities = queue_priorities;
    device_queue_create_infos.push_back(device_queue_create_info);

    VkPhysicalDeviceFeatures device_features {};
    device_features.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo device_create_info {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = &device_queue_create_info;
    device_create_info.pEnabledFeatures = &device_features;
    device_create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
    device_create_info.ppEnabledExtensionNames  = device_extensions.data();

    errorCheck(vkCreateDevice(gpu, &device_create_info, nullptr, &device));
    vkGetDeviceQueue(device, graphics_family_index, 0, &queue);

}

void Renderer::destroyDevice() {
    vkDestroyDevice(device, nullptr);
    device = nullptr;
}

#ifdef BUILD_ENABLE_VULKAN_DEBUG

VKAPI_ATTR VkBool32 VKAPI_CALL
VulkanDebugCallback (
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT	obj_type,
	uint64_t src_obj,
	size_t location,
	int32_t msg_code,
	const char *layer_prefix,
	const char *msg,
	void *user_data
) {
	std::ostringstream stream;
	stream << "VKDBG: ";
	if( flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT ) {
		stream << "INFO: ";
	}
	if( flags & VK_DEBUG_REPORT_WARNING_BIT_EXT ) {
		stream << "WARNING: ";
	}
	if( flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT ) {
		stream << "PERFORMANCE: ";
	}
	if( flags & VK_DEBUG_REPORT_ERROR_BIT_EXT ) {
		stream << "ERROR: ";
	}
	if( flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT ) {
		stream << "DEBUG: ";
	}
	stream << "@[" << layer_prefix << "]: ";
	stream << msg << std::endl;
	std::cout << stream.str();

	return false;
}

void Renderer::setupDebug() {
	debug_callback_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	debug_callback_create_info.pfnCallback = VulkanDebugCallback;
	debug_callback_create_info.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | 0;
	instance_layers.push_back("VK_LAYER_LUNARG_standard_validation");
	instance_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
}

PFN_vkCreateDebugReportCallbackEXT fvkCreateDebugReportCallbackEXT = nullptr;
PFN_vkDestroyDebugReportCallbackEXT fvkDestroyDebugReportCallbackEXT = nullptr;

void Renderer::initDebug() {
	fvkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT" );
	fvkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT" );
	if (nullptr == fvkCreateDebugReportCallbackEXT || nullptr == fvkDestroyDebugReportCallbackEXT) {
		assert(0 && "Vulkan ERROR: Can't fetch debug function pointers.");
		std::exit( -1 );
	}
	fvkCreateDebugReportCallbackEXT(instance, &debug_callback_create_info, nullptr, &debug_report);
}

void Renderer::destroyDebug() {
	fvkDestroyDebugReportCallbackEXT(instance, debug_report, nullptr);
	debug_report = VK_NULL_HANDLE;
}

#else

void Renderer::setupDebug() {};
void Renderer::initDebug() {};
void Renderer::destroyDebug() {};

#endif 

void Renderer::initSurface() {
    window->initOSSurface(this);
    VkBool32 WSI_supported = false;
    errorCheck(vkGetPhysicalDeviceSurfaceSupportKHR(gpu, graphics_family_index, surface, &WSI_supported));
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

void Renderer::destroySurface() {
    vkDestroySurfaceKHR(instance, surface, nullptr);
}

