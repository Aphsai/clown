#define VMA_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define BUILD_ENABLE_VULKAN_DEBUG 
#define BUILD_ENABLE_VULKAN_RUNTIME_DEBUG 

#include "stb_image.h"
#include "platform.hpp"
#include "renderer.hpp"
#include "shared.hpp"
#include "window.hpp"
#include "swapchain.hpp"

#include <cstdlib>
#include <assert.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <set>


Renderer::Renderer(uint32_t size_x, uint32_t size_y, std::string name) {
    surface_size_x = size_x;
    surface_size_y = size_y;
    window_name = name;
    

    initPlatform();
    setupLayersAndExtensions();
    setupDebug();
    initInstance();
    initDebug();
    initWindow();
    initDevice();
    initAllocator();
    initSwapchain();
    initRenderPass();
    initImageViews();
    initFramebuffers();
    initCommandBuffers();
}

Renderer::~Renderer() {
}

bool Renderer::run() {
    return window->update();
}

void Renderer::initWindow() {
    window = new Window(this, surface_size_x, surface_size_y, window_name);
}

void Renderer::destroyWindow() {
    delete window;
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
            if (present_family_index > -1 && graphics_family_index > -1) break;
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
    
    for (auto queue_family : unique_queue_families) {
        VkDeviceQueueCreateInfo device_queue_create_info {};
        device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        device_queue_create_info.queueFamilyIndex = queue_family;
        device_queue_create_info.queueCount = 1;
        device_queue_create_info.pQueuePriorities = queue_priorities;
        device_queue_create_infos.push_back(device_queue_create_info);
    }

    VkPhysicalDeviceFeatures device_features {};
    device_features.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo device_create_info {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = device_queue_create_infos.size();;
    device_create_info.pQueueCreateInfos = device_queue_create_infos.data();
    device_create_info.pEnabledFeatures = &device_features;
    device_create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
    device_create_info.ppEnabledExtensionNames  = device_extensions.data();

    errorCheck(vkCreateDevice(gpu, &device_create_info, nullptr, &device));
    vkGetDeviceQueue(device, graphics_family_index, 0, &graphics_queue);
    vkGetDeviceQueue(device, present_family_index, 0, &present_queue);

}

void Renderer::destroyDevice() {
    vkDestroyDevice(device, nullptr);
    device = nullptr;
}

void Renderer::initAllocator() {
    VmaAllocatorCreateInfo allocator_info {};
    allocator_info.physicalDevice = gpu;
    allocator_info.device = device;
    allocator_info.instance = instance;

    vmaCreateAllocator(&allocator_info, &allocator);
}

void Renderer::destroyAllocator() {
    vmaDestroyAllocator(allocator);
}

void Renderer::initSwapchain() {
    window->initOSSurface(this);
    VkBool32 WSI_supported = false;
    errorCheck(vkGetPhysicalDeviceSurfaceSupportKHR(gpu, graphics_family_index, surface, &WSI_supported));
    if (!WSI_supported) {
        assert(0 && "WSI not supported");
        std::exit(-1);
    }

    errorCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &surface_capabilities));
    swapchain_extent = Swapchain::chooseExtent(surface_capabilities);

    uint32_t format_count = 0;
    errorCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &format_count, nullptr));
    if (format_count == 0) {
        assert(0 && "Surface formats missing");
        std::exit(-1);
    }
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    errorCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &format_count, formats.data()));
    surface_format = Swapchain::chooseSwapchainFormat(formats);

    uint32_t present_mode_count = 0;
    std::vector<VkPresentModeKHR> present_modes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu,  surface, &present_mode_count, nullptr);
    present_modes.resize(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &present_mode_count, present_modes.data());
    present_mode = Swapchain::choosePresentMode(present_modes);
    
    swapchain_image_count = surface_capabilities.minImageCount;
    swapchain_image_count = std::max(surface_capabilities.maxImageCount, swapchain_image_count);

    VkSwapchainCreateInfoKHR create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface;
    create_info.minImageCount = swapchain_image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = swapchain_extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.preTransform = surface_capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;

    if (graphics_family_index != present_family_index) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = nullptr;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
    }

    errorCheck(vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain));
    vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, nullptr);
    swapchain_images.resize(swapchain_image_count);
    vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images.data());
    swapchain_format = surface_format.format;
}

void Renderer::destroySwapchain() {
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
}

void Renderer::initRenderPass() {
    VkAttachmentDescription color_attachment = {};
    color_attachment.format = swapchain_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    std::array<VkAttachmentDescription, 1> attachments = { color_attachment };

    VkRenderPassCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.attachmentCount = static_cast<uint32_t> (attachments.size());
    create_info.pAttachments = attachments.data();
    create_info.subpassCount = 1;
    create_info.pSubpasses = &subpass;

    errorCheck(vkCreateRenderPass(device, &create_info, nullptr, &render_pass));
}

void Renderer::destroyRenderPass() {
    vkDestroyRenderPass(device, render_pass, nullptr);
}

void Renderer::initImageViews() {
    image_views.resize(swapchain_images.size());
    for (size_t x = 0; x < swapchain_images.size(); x++) {
        VkImageViewCreateInfo view_info {};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = swapchain_images[x];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = swapchain_format;
        view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        errorCheck(vkCreateImageView(device, &view_info, nullptr, &image_views[x]));
    }
}

void Renderer::destroyImageViews() {
    for (auto image_view : image_views) {
        vkDestroyImageView(device, image_view, nullptr);
    }
}

void Renderer::initFramebuffers() {
    framebuffers.resize(swapchain_images.size());

    for (size_t x = 0; x < swapchain_images.size(); x++) {
        std::array<VkImageView, 1> attachments = { image_views[x] };

        VkFramebufferCreateInfo create_info {};
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = render_pass;
        create_info.attachmentCount = attachments.size();
        create_info.pAttachments = attachments.data();
        create_info.width = swapchain_extent.width;
        create_info.height = swapchain_extent.height;
        create_info.layers = 1;

        errorCheck(vkCreateFramebuffer(device, &create_info, nullptr, &framebuffers[x]));

    }
}

void Renderer::destroyFramebuffers() {
    for (auto framebuffer : framebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
}

void Renderer::initCommandBuffers() {
    VkCommandPoolCreateInfo create_pool_info {};
    create_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_pool_info.queueFamilyIndex = graphics_family_index;
    create_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    errorCheck(vkCreateCommandPool(device, &create_pool_info, nullptr, &command_pool));
    
    command_buffers.resize(swapchain_image_count);
    VkCommandBufferAllocateInfo create_buffer_info {};
    create_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    create_buffer_info.commandPool = command_pool;
    create_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    create_buffer_info.commandBufferCount = command_buffers.size();
    
    errorCheck(vkAllocateCommandBuffers(device, &create_buffer_info, command_buffers.data()));
}

void Renderer::destroyCommandBuffers() {
    vkFreeCommandBuffers(device, command_pool, command_buffers.size(), command_buffers.data());
    vkDestroyCommandPool(device, command_pool, nullptr); 
}

void Renderer::beginCommandBuffer(VkCommandBuffer command_buffer) {
    
    VkCommandBufferBeginInfo begin_info {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    errorCheck(vkBeginCommandBuffer(command_buffer, &begin_info));
}

void Renderer::endCommandBuffer(VkCommandBuffer command_buffer) {
    errorCheck(vkEndCommandBuffer(command_buffer));
}

void Renderer::beginRenderPass(std::array<VkClearValue, 1> clear_values, VkCommandBuffer command_buffer, VkFramebuffer framebuffer, VkExtent2D extent) {
    VkRenderPassBeginInfo begin_info {};
    begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin_info.renderPass = render_pass;
    begin_info.framebuffer = framebuffer;
    begin_info.renderArea.offset = { 0, 0 };
    begin_info.renderArea.extent = extent;
    begin_info.pClearValues = clear_values.data();
    begin_info.clearValueCount = clear_values.size();

    vkCmdBeginRenderPass(command_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void Renderer::endRenderPass(VkCommandBuffer command_buffer) {
    vkCmdEndRenderPass(command_buffer);
}

void Renderer::drawBegin() {
    vkAcquireNextImageKHR(device, swapchain, std::numeric_limits<uint64_t>::max(), nullptr, VK_NULL_HANDLE, &active_swapchain_image_id);
    beginCommandBuffer(command_buffers[active_swapchain_image_id]);
    VkClearValue clear_color { 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<VkClearValue, 1> clear_values = { clear_color };
    beginRenderPass(clear_values, command_buffers[active_swapchain_image_id], framebuffers[active_swapchain_image_id], swapchain_extent);
}

void Renderer::drawEnd() {
    endRenderPass(command_buffers[active_swapchain_image_id]);
    endCommandBuffer(command_buffers[active_swapchain_image_id]);
    VkSubmitInfo submit_info {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers[active_swapchain_image_id];
    vkQueueSubmit(graphics_queue, 1, &submit_info, nullptr);

    VkPresentInfoKHR present_info {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &active_swapchain_image_id;

    vkQueuePresentKHR(present_queue, &present_info);
    vkQueueWaitIdle(present_queue);
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
