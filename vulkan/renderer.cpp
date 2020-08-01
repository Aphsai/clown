#include "platform.hpp"
#include "renderer.hpp"
#include "shared.hpp"
#include "window.hpp"

#include <cstdlib>
#include <assert.h>
#include <vector>
#include <iostream>
#include <sstream>

Renderer::Renderer() {
    initPlatform();
    setupLayersAndExtensions();
    setupDebug();
    initInstance();
    initDebug();
    initDevice();
}

Renderer::~Renderer() {
    delete window;
    destroyDevice();
    destroyDebug();
    destroyInstance();
    destroyPlatform();
}

Window* Renderer::openWindow(uint32_t size_x, uint32_t size_y, std::string name) {
    window = new Window(this, size_x, size_y, name);
    return window;
}

bool Renderer::run() {
    if (nullptr != window) {
        return window->update();
    }
    return true;
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

    gpu = gpu_list[0];
    vkGetPhysicalDeviceProperties(gpu, &gpu_properties);
    vkGetPhysicalDeviceMemoryProperties(gpu, &gpu_memory_properties);

    uint32_t family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &family_count, nullptr);
    std::vector<VkQueueFamilyProperties> family_property_list(family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &family_count, family_property_list.data());

    bool found = false;
    for (uint32_t x = 0; x < family_count; x++) {
        if (family_property_list[x].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            found = true;
            graphics_family_index = x;
        }
    }

    if (!found) {
        assert(0 && "VULKAN ERROR: Queue family supporting graphics not found.");
        std::exit(-1);
    }

    float queue_priorities[] { 1.0f };
    VkDeviceQueueCreateInfo device_queue_create_info {};
    device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    device_queue_create_info.queueFamilyIndex = graphics_family_index;
    device_queue_create_info.queueCount = 1;
    device_queue_create_info.pQueuePriorities = queue_priorities;

    VkDeviceCreateInfo device_create_info {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = &device_queue_create_info;
    device_create_info.enabledExtensionCount = device_extensions.size();
    device_create_info.ppEnabledExtensionNames  = device_extensions.data();

    errorCheck(vkCreateDevice(gpu, &device_create_info, nullptr, &device));
    vkGetDeviceQueue(device, graphics_family_index, 0, &queue);

}

void Renderer::destroyDevice() {
    vkDestroyDevice(device, nullptr);
    device = nullptr;
}

#if BUILD_ENABLE_VULKAN_DEBUG

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
	_debug_callback_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	_debug_callback_create_info.pfnCallback = VulkanDebugCallback;
	_debug_callback_create_info.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | 0;
	_instance_layers.push_back("VK_LAYER_LUNARG_standard_validation");
	_instance_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
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
