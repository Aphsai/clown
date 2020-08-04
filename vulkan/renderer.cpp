#define VMA_IMPLEMENTATION
#include "platform.hpp"
#include "renderer.hpp"
#include "shared.hpp"
#include "window.hpp"
#include "vertex.hpp"

#include <cstdlib>
#include <assert.h>
#include <vector>
#include <iostream>
#include <sstream>

const std::vector<Vertex> vertices = { 
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}}, 
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}}, 
    {{-0.5, 0.5f}, {0.0f, 0.0f, 1.0f}} 
};


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

void Renderer::initialize() {
    initSurface();
    initSwapchain();
    initSwapchainImages();
    initRenderPass();
    initGraphicsPipeline();
    initFramebuffers();
    initCommandPool();
    initVMAAllocator();
    initVertexBuffer();
    initCommandBuffers();
    initSynchronizations();
}

void Renderer::destroy() {
    vkQueueWaitIdle(queue);
    destroySynchronizations();
    destroyCommandBuffers();
    destroyVertexBuffer();
    destroyVMAAllocator();
    destroyCommandPool();
    destroyFramebuffers();
    destroyGraphicsPipeline();
    destroyRenderPass();
    destroySwapchainImages();
    destroySwapchain();
    destroySurface();
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
	swapchain_create_info.imageExtent.width	= surface_size_x;
	swapchain_create_info.imageExtent.height = surface_size_y;
	swapchain_create_info.imageArrayLayers = 1;
	swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
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
    swapchain_images.resize(swapchain_image_count);
    swapchain_image_views.resize(swapchain_image_count);

    errorCheck(vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images.data()));
    for (uint32_t x = 0; x < swapchain_image_count; x++) {
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

	VkImageCreateInfo image_create_info {};
	image_create_info.sType					= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.flags					= 0;
	image_create_info.imageType				= VK_IMAGE_TYPE_2D;
	image_create_info.format				= depth_stencil_format;
	image_create_info.extent.width			= surface_size_x;
	image_create_info.extent.height			= surface_size_y;
	image_create_info.extent.depth			= 1;
	image_create_info.mipLevels				= 1;
	image_create_info.arrayLayers			= 1;
	image_create_info.samples				= VK_SAMPLE_COUNT_1_BIT;
	image_create_info.tiling				= VK_IMAGE_TILING_OPTIMAL;
	image_create_info.usage					= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	image_create_info.sharingMode			= VK_SHARING_MODE_EXCLUSIVE;
	image_create_info.queueFamilyIndexCount	= VK_QUEUE_FAMILY_IGNORED;
	image_create_info.pQueueFamilyIndices	= nullptr;
	image_create_info.initialLayout			= VK_IMAGE_LAYOUT_UNDEFINED;

	errorCheck(vkCreateImage(device, &image_create_info, nullptr, &depth_stencil_image));

	VkMemoryRequirements image_memory_requirements {};
	vkGetImageMemoryRequirements(device, depth_stencil_image, &image_memory_requirements);

	uint32_t memory_index = findMemoryTypeIndex(&(gpu_memory_properties), &image_memory_requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkMemoryAllocateInfo memory_allocate_info {};
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize = image_memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = memory_index;

	errorCheck(vkAllocateMemory(device, &memory_allocate_info, nullptr, &depth_stencil_image_memory));
	errorCheck(vkBindImageMemory(device, depth_stencil_image, depth_stencil_image_memory, 0));

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
	vkDestroyImageView(device, depth_stencil_image_view, nullptr);
	vkFreeMemory(device, depth_stencil_image_memory, nullptr);
	vkDestroyImage(device, depth_stencil_image, nullptr);
}

void Renderer::initRenderPass() {
	std::array<VkAttachmentDescription, 1> attachments {};
	attachments[0].flags = 0;
	attachments[0].format = surface_format.format;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	std::array<VkAttachmentReference, 1> sub_pass_0_color_attachments {};
	sub_pass_0_color_attachments[0].attachment = 0;
	sub_pass_0_color_attachments[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	std::array<VkSubpassDescription, 1> sub_passes {};
	sub_passes[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	sub_passes[0].colorAttachmentCount = sub_pass_0_color_attachments.size();
	sub_passes[0].pColorAttachments	= sub_pass_0_color_attachments.data();	

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
	framebuffers.resize(swapchain_image_count);
	for (uint32_t x = 0; x < swapchain_image_count; x++) {
		std::array<VkImageView, 1> attachments {};
		attachments[0]	= swapchain_image_views[x];

		VkFramebufferCreateInfo framebuffer_create_info {};
		framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_create_info.renderPass = render_pass;
		framebuffer_create_info.attachmentCount	= attachments.size();
		framebuffer_create_info.pAttachments = attachments.data();
		framebuffer_create_info.width = surface_size_x;
		framebuffer_create_info.height = surface_size_y;
		framebuffer_create_info.layers = 1;

		errorCheck(vkCreateFramebuffer(device, &framebuffer_create_info, nullptr, &framebuffers[x]));
	}
}

void Renderer::destroyFramebuffers() {
	for (auto f : framebuffers) {
		vkDestroyFramebuffer(device, f, nullptr);
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
    pipeline_layout_info.setLayoutCount = 0;
    pipeline_layout_info.pSetLayouts = nullptr;
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
    pool_info.queueFamilyIndex = graphics_family_index;
    pool_info.flags = 0;
    errorCheck(vkCreateCommandPool(device, &pool_info, nullptr, &command_pool));
}

void Renderer::destroyCommandPool() {
    vkDestroyCommandPool(device, command_pool, nullptr);
}

void Renderer::initCommandBuffers() {
    command_buffers.resize(framebuffers.size());
    VkCommandBufferAllocateInfo alloc_info {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = (uint32_t) command_buffers.size();

    errorCheck(vkAllocateCommandBuffers(device, &alloc_info, command_buffers.data()));

    for (size_t x = 0; x < command_buffers.size(); x++) {
        VkCommandBufferBeginInfo begin_info {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = 0;
        begin_info.pInheritanceInfo = nullptr;

        errorCheck(vkBeginCommandBuffer(command_buffers[x], &begin_info));

        VkRenderPassBeginInfo render_pass_info {};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = render_pass;
        render_pass_info.framebuffer = framebuffers[x];
        render_pass_info.renderArea.offset = {0, 0};
        render_pass_info.renderArea.extent = swapchain_extent;

        VkClearValue clear_color = { 0.0f, 0.0f, 0.0f, 0.0f };
        render_pass_info.clearValueCount = 1;
        render_pass_info.pClearValues = &clear_color;

        vkCmdBeginRenderPass(command_buffers[x], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(command_buffers[x], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
        VkBuffer vertex_buffers[] = { vertex_buffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(command_buffers[x], 0, 1, vertex_buffers, offsets);
        vkCmdDraw(command_buffers[x], static_cast<uint32_t>(vertices.size()), 1, 0, 0);
        vkCmdEndRenderPass(command_buffers[x]);

        errorCheck(vkEndCommandBuffer(command_buffers[x]));
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

    VkBufferCopy copy_region {};
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = buffer_size;
    vkCmdCopyBuffer(command_buffer, staging_buffer, vertex_buffer, 1, &copy_region);

    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
    vmaDestroyBuffer(allocator, staging_buffer, staging_allocation);
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
