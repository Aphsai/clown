#include "window.hpp"
#include "renderer.hpp"
#include "shared.hpp"
#include "BUILD_OPTIONS.hpp"
 
#include <assert.h>
#include <array>

Window::Window(Renderer* render, uint32_t size_x, uint32_t size_y, std::string name) {
    renderer = render;
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
    initSynchronizations();
}

Window::~Window() {
    vkQueueWaitIdle(renderer->queue);
    destroySynchronizations();
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

void Window::initOSWindow() {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfw_window = glfwCreateWindow(surface_size_x, surface_size_y, window_name.c_str(), nullptr, nullptr);
    if (!glfw_window) {
        glfwTerminate();
        assert(0 && "GLFW could not create window.");
        return;
    }
    glfwGetFramebufferSize(glfw_window, (int*)&surface_size_x, (int*)&surface_size_y);
}

void Window::destroyOSWindow() {
    glfwDestroyWindow(glfw_window);
}

void Window::updateOSWindow() {
    glfwPollEvents();
    if (glfwWindowShouldClose(glfw_window)) close();
}

void Window::initOSSurface() {
    if (VK_SUCCESS != glfwCreateWindowSurface(renderer->instance, glfw_window, nullptr, &surface)) {
        glfwTerminate();
        assert(0 && "GLFW could not create window surface.");
        return;
    }
}

void Window::initSwapchain() {
    if (swapchain_image_count < surface_capabilities.minImageCount + 1) swapchain_image_count = surface_capabilities.minImageCount + 1;
    if (surface_capabilities.maxImageCount > 0) {
        if (swapchain_image_count > surface_capabilities.maxImageCount) swapchain_image_count = surface_capabilities.maxImageCount;
    }

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    uint32_t present_mode_count = 0;
    errorCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(renderer->gpu, surface, &present_mode_count, nullptr));
    std::vector<VkPresentModeKHR> present_mode_list(present_mode_count);
    errorCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(renderer->gpu, surface, &present_mode_count, present_mode_list.data()));
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

	errorCheck(vkCreateSwapchainKHR(renderer->device, &swapchain_create_info, nullptr, &swapchain));
	errorCheck(vkGetSwapchainImagesKHR(renderer->device, swapchain, &swapchain_image_count, nullptr ));
}

void Window::destroySwapchain() {
    vkDestroySwapchainKHR(renderer->device, swapchain, nullptr);
}


void Window::initSwapchainImages() {
    swapchain_images.resize(swapchain_image_count);
    swapchain_image_views.resize(swapchain_image_count);

    errorCheck(vkGetSwapchainImagesKHR(renderer->device, swapchain, &swapchain_image_count, swapchain_images.data()));
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
        errorCheck(vkCreateImageView(renderer->device, &image_view_create_info, nullptr, &swapchain_image_views[x]));
    }
}

void Window::destroySwapchainImages() {
    for(auto view : swapchain_image_views) {
		vkDestroyImageView(renderer->device, view, nullptr);
	}
}

void Window::initDepthStencilImage() {
	std::vector<VkFormat> try_formats {
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D16_UNORM
	};
	for(auto f : try_formats) {
		VkFormatProperties format_properties {};
		vkGetPhysicalDeviceFormatProperties(renderer->gpu, f, &format_properties);
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

	errorCheck( vkCreateImage(renderer->device, &image_create_info, nullptr, &depth_stencil_image));

	VkMemoryRequirements image_memory_requirements {};
	vkGetImageMemoryRequirements(renderer->device, depth_stencil_image, &image_memory_requirements);

	uint32_t memory_index = findMemoryTypeIndex(&(renderer->gpu_memory_properties), &image_memory_requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkMemoryAllocateInfo memory_allocate_info {};
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize = image_memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = memory_index;

	errorCheck(vkAllocateMemory(renderer->device, &memory_allocate_info, nullptr, &depth_stencil_image_memory));
	errorCheck(vkBindImageMemory(renderer->device, depth_stencil_image, depth_stencil_image_memory, 0));

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

	errorCheck(vkCreateImageView(renderer->device, &image_view_create_info, nullptr, &depth_stencil_image_view));
}

void Window::destroyDepthStencilImage() {
	vkDestroyImageView(renderer->device, depth_stencil_image_view, nullptr);
	vkFreeMemory(renderer->device, depth_stencil_image_memory, nullptr);
	vkDestroyImage(renderer->device, depth_stencil_image, nullptr);
}

void Window::initRenderPass() {
	std::array<VkAttachmentDescription, 2> attachments {};
	attachments[0].flags = 0;
	attachments[0].format = depth_stencil_format;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	attachments[1].flags = 0;
	attachments[1].format = surface_format.format;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;


	VkAttachmentReference sub_pass_0_depth_stencil_attachment {};
	sub_pass_0_depth_stencil_attachment.attachment = 0;
	sub_pass_0_depth_stencil_attachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	std::array<VkAttachmentReference, 1> sub_pass_0_color_attachments {};
	sub_pass_0_color_attachments[0].attachment = 1;
	sub_pass_0_color_attachments[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	std::array<VkSubpassDescription, 1> sub_passes {};
	sub_passes[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	sub_passes[0].colorAttachmentCount = sub_pass_0_color_attachments.size();
	sub_passes[0].pColorAttachments	= sub_pass_0_color_attachments.data();	
	sub_passes[0].pDepthStencilAttachment = &sub_pass_0_depth_stencil_attachment;


	VkRenderPassCreateInfo render_pass_create_info {};
	render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.attachmentCount	= attachments.size();
	render_pass_create_info.pAttachments = attachments.data();
	render_pass_create_info.subpassCount = sub_passes.size();
	render_pass_create_info.pSubpasses = sub_passes.data();

	errorCheck(vkCreateRenderPass(renderer->device, &render_pass_create_info, nullptr, &render_pass));
}

void Window::destroyRenderPass() {
	vkDestroyRenderPass(renderer->device, render_pass, nullptr);
}

void Window::initFramebuffers() {
	framebuffers.resize(swapchain_image_count);
	for (uint32_t x = 0; x < swapchain_image_count; x++) {
		std::array<VkImageView, 2> attachments {};
		attachments[0]	= depth_stencil_image_view;
		attachments[1]	= swapchain_image_views[x];

		VkFramebufferCreateInfo framebuffer_create_info {};
		framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_create_info.renderPass = render_pass;
		framebuffer_create_info.attachmentCount	= attachments.size();
		framebuffer_create_info.pAttachments = attachments.data();
		framebuffer_create_info.width = surface_size_x;
		framebuffer_create_info.height = surface_size_y;
		framebuffer_create_info.layers = 1;

		errorCheck(vkCreateFramebuffer(renderer->device, &framebuffer_create_info, nullptr, &framebuffers[x]));
	}
}

void Window::destroyFramebuffers() {
	for (auto f : framebuffers) {
		vkDestroyFramebuffer(renderer->device, f, nullptr);
	}
}

void Window::initSynchronizations() {
	VkFenceCreateInfo fence_create_info {};
	fence_create_info.sType	= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	vkCreateFence(renderer->device, &fence_create_info, nullptr, &swapchain_image_available);
}

void Window::destroySynchronizations() {
	vkDestroyFence(renderer->device, swapchain_image_available, nullptr);
}
