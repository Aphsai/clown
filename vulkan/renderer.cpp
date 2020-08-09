#define VMA_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define BUILD_ENABLE_VULKAN_DEBUG 
#define BUILD_ENABLE_VULKAN_RUNTIME_DEBUG 

#include "stb_image.h"
#include "platform.hpp"
#include "renderer.hpp"
#include "shared.hpp"
#include "window.hpp"
#include "vertex.hpp"
#include "ubo.hpp"

#include <cstdlib>
#include <assert.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <set>

const std::vector<Vertex> vertices = { 
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, 
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, 
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, 
    {{-0.5, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}} 
};


Renderer::Renderer(uint32_t size_x, uint32_t size_y, std::string name) {
    initPlatform();
    setupLayersAndExtensions();
    setupDebug();
    initInstance();
    initDebug();

    window = new Window(this, size_x, size_y, name);

    initDevice();
    initSurface();
    initSwapchain();
    initSwapchainImages();
    initRenderPass();
    initDescriptorSetLayout();
    initGraphicsPipeline();
    initFramebuffers();
    initCommandPool();
    initVMAAllocator();
    initTextureImage();
    initTextureImageView();
    initTextureSampler();
    initVertexBuffer();
    initIndexBuffer();
    initUniformBuffer();
    initDescriptorPool();
    initDescriptorSets();
    initCommandBuffers();
    initSynchronizations();
}

Renderer::~Renderer() {
    vkDeviceWaitIdle(device);

    destroySynchronizations();
    destroyCommandBuffers();
    destroyTextureSampler();
    destroyTextureImageView();
    destroyTextureImage();
    destroyVertexBuffer();
    destroyVMAAllocator();
    destroyCommandPool();
    destroyFramebuffers();
    destroyGraphicsPipeline();

    destroyRenderPass();
    destroySwapchainImages();
    destroySwapchain();
    destroySurface();

    delete window;

    destroyDevice();
    destroyDebug();
    destroyInstance();
    destroyPlatform();
}

void Renderer::recreateSwapchain() {
    vkDeviceWaitIdle(device);

    destroyCommandBuffers();
    destroyFramebuffers();
    destroyGraphicsPipeline();
    destroyRenderPass();
    destroySwapchainImages();
    destroySwapchain();

    initSwapchain();
    initSwapchainImages();
    initRenderPass();
    initGraphicsPipeline();
    initFramebuffers();
    initUniformBuffer();
    initCommandBuffers();
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

void Renderer::beginRender() {
    VkResult result;
    errorCheck(result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, VK_NULL_HANDLE, swapchain_image_available, &active_swapchain_image_id));
    if (result == VK_ERROR_OUT_OF_DATE_KHR) recreateSwapchain();
    errorCheck(vkWaitForFences(device, 1, &swapchain_image_available, VK_TRUE, UINT64_MAX));
    errorCheck(vkResetFences(device, 1, &swapchain_image_available));
    errorCheck(vkQueueWaitIdle(queue));

}

void Renderer::endRender(std::vector<VkSemaphore> wait_semaphores) {
    VkResult present_result = VkResult::VK_RESULT_MAX_ENUM;
    VkPresentInfoKHR present_info {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = wait_semaphores.size();
    present_info.pWaitSemaphores = wait_semaphores.data();
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &active_swapchain_image_id;
    present_info.pResults = &present_result;

    errorCheck(vkQueuePresentKHR(queue, &present_info));
    errorCheck(present_result);
}

void Renderer::initSurface() {
    window->initOSSurface();
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

void Renderer::initSwapchain() {
    if (swapchain_image_count < surface_capabilities.minImageCount + 1) {
        swapchain_image_count = surface_capabilities.minImageCount + 1;
    }
    if (surface_capabilities.maxImageCount > 0) {
        if (swapchain_image_count > surface_capabilities.maxImageCount) {
            swapchain_image_count = surface_capabilities.maxImageCount;
        }
    }

    if (surface_capabilities.currentExtent.width != UINT32_MAX) {
        swapchain_extent = surface_capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window->glfw_window, &width, &height);
        swapchain_extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
        swapchain_extent.width = std::max(surface_capabilities.minImageExtent.width, std::min(surface_capabilities.maxImageExtent.width, swapchain_extent.width));
        swapchain_extent.height = std::max(surface_capabilities.minImageExtent.height, std::min(surface_capabilities.maxImageExtent.height, swapchain_extent.height));
    }

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    uint32_t present_mode_count = 0;
    errorCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &present_mode_count, nullptr));
    std::vector<VkPresentModeKHR> present_mode_list(present_mode_count);
    errorCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &present_mode_count, present_mode_list.data()));
    for (auto m : present_mode_list) {
        if (m == VK_PRESENT_MODE_MAILBOX_KHR) present_mode = m;
    }

    VkSwapchainCreateInfoKHR swapchain_create_info {};
	swapchain_create_info.sType	= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_create_info.surface = surface;
	swapchain_create_info.minImageCount = swapchain_image_count;
	swapchain_create_info.imageFormat = surface_format.format;
	swapchain_create_info.imageColorSpace = surface_format.colorSpace;
	swapchain_create_info.imageExtent = swapchain_extent;
	swapchain_create_info.imageArrayLayers = 1;
	swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_create_info.queueFamilyIndexCount = 0;
	swapchain_create_info.pQueueFamilyIndices = nullptr;
	swapchain_create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_create_info.presentMode = present_mode;
	swapchain_create_info.clipped = VK_TRUE;
	swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;

	errorCheck(vkCreateSwapchainKHR(device, &swapchain_create_info, nullptr, &swapchain));
	errorCheck(vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, nullptr ));
}

void Renderer::destroySwapchain() {
    vkDestroySwapchainKHR(device, swapchain, nullptr);
}


void Renderer::initSwapchainImages() {
    swapchain_images.resize(NUM_FRAME_DATA);
    swapchain_image_views.resize(NUM_FRAME_DATA);

    errorCheck(vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images.data()));
    for (uint32_t x = 0; x < NUM_FRAME_DATA; x++) {
        VkImageViewCreateInfo image_view_create_info {};
        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.image = swapchain_images[x];
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.format = surface_format.format;
        image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_create_info.subresourceRange.baseMipLevel = 0;
        image_view_create_info.subresourceRange.levelCount = 1;
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.layerCount = 1;
        errorCheck(vkCreateImageView(device, &image_view_create_info, nullptr, &swapchain_image_views[x]));

        //TODO: OFFLOAD SWAPCHAIN IMAGES TO IMAGE CLASS?
    }
}

void Renderer::destroySwapchainImages() {
    for(auto view : swapchain_image_views) {
		vkDestroyImageView(device, view, nullptr);
	}
}

void Renderer::initDepthStencilImage() {
	std::vector<VkFormat> try_formats {
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D16_UNORM
	};
	for(auto f : try_formats) {
		VkFormatProperties format_properties {};
		vkGetPhysicalDeviceFormatProperties(gpu, f, &format_properties);
		if(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			depth_stencil_format = f;
			break;
		}
	}
	if(depth_stencil_format == VK_FORMAT_UNDEFINED) {
		assert( 0 && "Depth stencil format not selected." );
		std::exit(-1);
	}
	if( (depth_stencil_format == VK_FORMAT_D32_SFLOAT_S8_UINT) ||
		(depth_stencil_format == VK_FORMAT_D24_UNORM_S8_UINT) ||
		(depth_stencil_format == VK_FORMAT_D16_UNORM_S8_UINT) ||
		(depth_stencil_format == VK_FORMAT_S8_UINT)) {
		stencil_available = true;
	}

    depth_stencil_allocation = createImage(swapchain_extent.width, swapchain_extent.height, depth_stencil_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth_stencil_image);

	VkImageViewCreateInfo image_view_create_info {};
	image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_create_info.image = depth_stencil_image;
	image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	image_view_create_info.format = depth_stencil_format;
	image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.components.a	= VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | (stencil_available ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
	image_view_create_info.subresourceRange.baseMipLevel = 0;
	image_view_create_info.subresourceRange.levelCount = 1;
	image_view_create_info.subresourceRange.baseArrayLayer = 0;
	image_view_create_info.subresourceRange.layerCount = 1;

	errorCheck(vkCreateImageView(device, &image_view_create_info, nullptr, &depth_stencil_image_view));
}

void Renderer::destroyDepthStencilImage() {
	vmaDestroyImage(allocator, depth_stencil_image, depth_stencil_allocation);
}

void Renderer::initRenderPass() {
	std::array<VkAttachmentDescription, 2> attachments {};
    // Color
	attachments[0].flags = 0;
	attachments[0].format = surface_format.format;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    // Depth stencil
	attachments[1].format = depth_stencil_format;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	std::array<VkAttachmentReference, 1> sub_pass_0_color_attachments {};
	sub_pass_0_color_attachments[0].attachment = 0;
	sub_pass_0_color_attachments[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_stencil_reference {};
    depth_stencil_reference.attachment = 1;
	depth_stencil_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	std::array<VkSubpassDescription, 1> sub_passes {};
	sub_passes[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	sub_passes[0].colorAttachmentCount = sub_pass_0_color_attachments.size();
	sub_passes[0].pColorAttachments	= sub_pass_0_color_attachments.data();	
    sub_passes[0].pDepthStencilAttachment = &depth_stencil_reference;

    VkSubpassDependency dependency {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo render_pass_create_info {};
	render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.attachmentCount	= attachments.size();
	render_pass_create_info.pAttachments = attachments.data();
	render_pass_create_info.subpassCount = sub_passes.size();
	render_pass_create_info.pSubpasses = sub_passes.data();
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &dependency;

	errorCheck(vkCreateRenderPass(device, &render_pass_create_info, nullptr, &render_pass));
}

void Renderer::destroyRenderPass() {
	vkDestroyRenderPass(device, render_pass, nullptr);
}

void Renderer::initFramebuffers() {
	framebuffers.resize(NUM_FRAME_DATA);
	std::array<VkImageView, 2> attachments {};
    assert(depth_stencil_image && "No depth stencil image");

	for (uint32_t x = 0; x < swapchain_image_count; x++) {
	    VkFramebufferCreateInfo framebuffer_create_info {};
	    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	    framebuffer_create_info.renderPass = render_pass;
	    framebuffer_create_info.attachmentCount	= attachments.size();
	    framebuffer_create_info.pAttachments = attachments.data();
	    framebuffer_create_info.width = surface_size_x;
	    framebuffer_create_info.height = surface_size_y;
	    framebuffer_create_info.layers = 1;
	    attachments[0]	= swapchain_image_views[x];
        attachments[1] = depth_stencil_image_view;
		errorCheck(vkCreateFramebuffer(device, &framebuffer_create_info, nullptr, &framebuffers[x]));
	}
}

void Renderer::destroyFramebuffers() {
	for (auto f : framebuffers) {
		vkDestroyFramebuffer(device, f, nullptr);
	}
}

void Renderer::initSemaphores() {
    acquire_semaphores.resize(NUM_FRAME_DATA);
    render_complete_semaphores.resize(NUM_FRAME_DATA);

    VkSemaphoreCreateInfo semaphore_create_info {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (int x = 0; x < NUM_FRAME_DATA; x++) {
        errorCheck(vkCreateSemaphore(device, &semaphore_create_info, nullptr, &acquire_semaphores[x]));
        errorCheck(vkCreateSemaphore(device, &semaphore_create_info, nullptr, &render_complete_semaphores[x]));
    }
}

void Renderer::initSynchronizations() {
	VkFenceCreateInfo fence_create_info {};
	fence_create_info.sType	= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	vkCreateFence(device, &fence_create_info, nullptr, &swapchain_image_available);
}

void Renderer::destroySynchronizations() {
	vkDestroyFence(device, swapchain_image_available, nullptr);
}


void Renderer::initGraphicsPipeline() {
    auto vert_shader_code = readFile("shaders/vert.spv");
    auto frag_shader_code = readFile("shaders/frag.spv");

    auto binding_description = Vertex::getBindingDescription();
    auto attribute_descriptions = Vertex::getAttributeDescriptions();
    
    VkShaderModule vert_shader_module = createShaderModule(vert_shader_code);
    VkShaderModule frag_shader_module = createShaderModule(frag_shader_code);

    VkPipelineShaderStageCreateInfo vert_shader_stage_info {};
    vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_info.module = vert_shader_module;
    vert_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_shader_stage_info {};
    frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_info.module = frag_shader_module;
    frag_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info, frag_shader_stage_info };

    // Vertex Input
    VkPipelineVertexInputStateCreateInfo vertex_input_info {};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
    vertex_input_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

    // Input Assembly
    VkPipelineInputAssemblyStateCreateInfo input_assembly {};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    // Viewport
    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapchain_extent.width;
    viewport.height = (float) swapchain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Scissor
    VkRect2D scissor {};
    scissor.offset = {0, 0};
    scissor.extent = swapchain_extent;

    VkPipelineViewportStateCreateInfo viewport_state {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    // Color blending
    VkPipelineColorBlendAttachmentState color_blend_attachment {};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blending {};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;
    color_blending.blendConstants[0] = 0.0f;
    color_blending.blendConstants[1] = 0.0f;
    color_blending.blendConstants[2] = 0.0f;
    color_blending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipeline_layout_info {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &descriptor_set_layout;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = 0;

    errorCheck(vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout));

    VkGraphicsPipelineCreateInfo pipeline_info {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = nullptr;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = nullptr;
    pipeline_info.layout = pipeline_layout;
    pipeline_info.renderPass = render_pass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

    errorCheck(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline));

    vkDestroyShaderModule(device, frag_shader_module, nullptr);
    vkDestroyShaderModule(device, vert_shader_module, nullptr);
}

void Renderer::destroyGraphicsPipeline() {
    vkDestroyPipeline(device, graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
}

void Renderer::initCommandPool() {
    VkCommandPoolCreateInfo pool_info {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = graphics_family_index;
    errorCheck(vkCreateCommandPool(device, &pool_info, nullptr, &command_pool));
}

void Renderer::destroyCommandPool() {
    vkDestroyCommandPool(device, command_pool, nullptr);
}

void Renderer::initCommandBuffers() {
    command_buffers.resize(NUM_FRAME_DATA);

    VkCommandBufferAllocateInfo alloc_info {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = (uint32_t) command_buffers.size();

    errorCheck(vkAllocateCommandBuffers(device, &alloc_info, command_buffers.data()));
    
    VkFenceCreateInfo fence_create_info {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    for (size_t x = 0; x < command_buffers.size(); x++) {
        errorCheck(vkCreateFence(device, &fence_create_info, nullptr, &command_buffer_fences[x]));
    }
}

void Renderer::destroyCommandBuffers() {
    vkFreeCommandBuffers(device, command_pool, static_cast<uint32_t>(command_buffers.size()), command_buffers.data());
}

void Renderer::initVertexBuffer() {
    VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();
    
    VkBuffer staging_buffer;
    VmaAllocation staging_allocation = createBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer);

    void* data; 
    vmaMapMemory(allocator, staging_allocation, &data);
    memcpy(data, vertices.data(), (size_t) buffer_size);
    vmaUnmapMemory(allocator, staging_allocation);

    allocation = createBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_buffer);

    // Copy buffers
    VkCommandBuffer command_buffer = beginSingleCommand();

    VkBufferCopy copy_region {};
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = buffer_size;
    vkCmdCopyBuffer(command_buffer, staging_buffer, vertex_buffer, 1, &copy_region);

    endSingleCommand(command_buffer);
    vmaDestroyBuffer(allocator, staging_buffer, staging_allocation);
}

VkCommandBuffer Renderer::beginSingleCommand() {
    VkCommandBufferAllocateInfo allocate_info {};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandPool = command_pool;
    allocate_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(device, &allocate_info, &command_buffer);
    
    VkCommandBufferBeginInfo begin_info {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &begin_info);

    return command_buffer;
}

void Renderer::endSingleCommand(VkCommandBuffer command_buffer) {
    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}


void Renderer::destroyVertexBuffer() {
    vmaDestroyBuffer(allocator, vertex_buffer, allocation);
}

void Renderer::initVMAAllocator() {
    VmaAllocatorCreateInfo allocator_info {};
    allocator_info.physicalDevice = gpu;
    allocator_info.device = device;
    allocator_info.instance = instance;

    vmaCreateAllocator(&allocator_info, &allocator);
}

void Renderer::destroyVMAAllocator() {
    vmaDestroyAllocator(allocator);
}

void Renderer::initTextureImage() {
    int width, height, channels;
    stbi_uc* pixels = stbi_load("assets/tilesheet.png", &width, &height, &channels, STBI_rgb_alpha);
    VkDeviceSize image_size = width * height * 4;
    if (!pixels) throw std::runtime_error("failed to load texture!");

    VkBuffer staging_buffer;
    VmaAllocation staging_allocation = createBuffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer);

    void* data; 
    vmaMapMemory(allocator, staging_allocation, &data);
    memcpy(data, pixels, (size_t) image_size);
    vmaUnmapMemory(allocator, staging_allocation);

    stbi_image_free(pixels);

    //createImage(width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture_image, texture_image_memory);
    transitionLayout(texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(staging_buffer, texture_image, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    transitionLayout(texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vmaDestroyBuffer(allocator, staging_buffer, staging_allocation);
    
}

void Renderer::destroyTextureImage() {
    vkDestroyImage(device, texture_image, nullptr);
    vkFreeMemory(device, texture_image_memory, nullptr);
}

void Renderer::initTextureImageView() {
    VkImageViewCreateInfo view_info {};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = texture_image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = VK_FORMAT_R8G8B8A8_SRGB;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    errorCheck(vkCreateImageView(device, &view_info, nullptr, &texture_image_view));
}

void Renderer::destroyTextureImageView() {
    vkDestroyImageView(device, texture_image_view, nullptr);
}

void Renderer::initTextureSampler() {
    VkSamplerCreateInfo sampler_info {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy = 16.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 0.0f;

    errorCheck(vkCreateSampler(device, &sampler_info, nullptr, &texture_sampler));
}

void Renderer::destroyTextureSampler() {
    vkDestroySampler(device, texture_sampler, nullptr);
}

void Renderer::initDescriptorSetLayout() {
	std::array<VkDescriptorSetLayoutBinding, 2> layout_bindings {};
    layout_bindings[0].binding = 0;
    layout_bindings[0].descriptorCount = 1;
    layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layout_bindings[0].pImmutableSamplers = nullptr;
    layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    layout_bindings[1].binding = 1;
    layout_bindings[1].descriptorCount = 1;
    layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layout_bindings[1].pImmutableSamplers = nullptr;
    layout_bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;


    VkDescriptorSetLayoutCreateInfo layout_info {};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = static_cast<uint32_t>(layout_bindings.size());
    layout_info.pBindings = layout_bindings.data();

    errorCheck(vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &descriptor_set_layout));
}

void Renderer::initDescriptorPool() {
    VkDescriptorPoolSize pool_size;
    pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_size.descriptorCount = static_cast<uint32_t>(swapchain_images.size());

    VkDescriptorPoolCreateInfo pool_info {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;
    pool_info.maxSets = static_cast<uint32_t>(swapchain_images.size());

    errorCheck(vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptor_pool));
}

void Renderer::initDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(swapchain_images.size(), descriptor_set_layout);
    VkDescriptorSetAllocateInfo allocate_info {};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = descriptor_pool;
    allocate_info.descriptorSetCount = static_cast<uint32_t>(swapchain_images.size());
    allocate_info.pSetLayouts = layouts.data();

    descriptor_sets.resize(swapchain_images.size());
    errorCheck(vkAllocateDescriptorSets(device, &allocate_info, descriptor_sets.data()));

    for (size_t x = 0; x < swapchain_images.size(); x++) {
        VkDescriptorImageInfo image_info {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = texture_image_view;
        image_info.sampler = texture_sampler;

        VkWriteDescriptorSet descriptor_write;
        descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write.dstSet = descriptor_sets[x];
        descriptor_write.dstBinding = 0;
        descriptor_write.dstArrayElement = 0;
        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_write.descriptorCount = 1;
        descriptor_write.pImageInfo = &image_info;
        descriptor_write.pNext = nullptr;

        vkUpdateDescriptorSets(device, 1, &descriptor_write, 0, nullptr);
    }
}

void Renderer::initIndexBuffer() { }

void Renderer::initUniformBuffer() { }

VkShaderModule Renderer::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule shader_module;
    errorCheck(vkCreateShaderModule(device, &create_info, nullptr, &shader_module));
    return shader_module;
}


VmaAllocation Renderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer) {
    VkBufferCreateInfo buffer_info {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocate_info {};
    allocate_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocate_info.requiredFlags = properties;
    
    VmaAllocation allocation;
    errorCheck(vmaCreateBuffer(allocator, &buffer_info, &allocate_info, &buffer, &allocation, nullptr));
    return allocation;
}

VmaAllocation Renderer::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image) {
    VkImageCreateInfo image_info {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = static_cast<uint32_t>(width);
    image_info.extent.height = static_cast<uint32_t>(height);
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = usage;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VmaAllocationCreateInfo allocate_info {};
    allocate_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocate_info.requiredFlags = properties;
    
    VmaAllocation allocation;
    errorCheck(vmaCreateImage(allocator, &image_info, &allocate_info, &texture_image, &allocation, nullptr));
    return allocation;
}

void Renderer::transitionLayout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout) {
    VkCommandBuffer command_buffer = beginSingleCommand();
    VkImageMemoryBarrier barrier {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags source_stage, destination_stage;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {

        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {

        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    } else {
        throw std::invalid_argument("unsupported layout transition");
    }

    vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    endSingleCommand(command_buffer);
}

void Renderer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer command_buffer = beginSingleCommand();
    
    VkBufferImageCopy region {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    
    endSingleCommand(command_buffer);
}
