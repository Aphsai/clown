#include "vk_engine.hpp"
#include "vk_types.hpp"
#include "vk_init.hpp"
#include "platform.hpp"

#include <math.h>

void VulkanEngine::init() {
    initPlatform();
    initVulkan();
    initSwapchain();
    initCommands();
    initRenderPass();
    initFramebuffers();
    initSyncStructures();


    _is_initialized = true;
}

void VulkanEngine::initVulkan() {
    vkb::InstanceBuilder builder;
    auto inst_ret = builder.set_app_name("Vulkan")
        .request_validation_layers(true)
        .require_api_version(1,1,0)
        .use_default_debug_messenger()
        .build();
    vkb::Instance vkb_inst = inst_ret.value();

    _instance = vkb_inst.instance;
    _debug_messenger = vkb_inst.debug_messenger;

    window = new Window(this, "Jester");

    vkb::PhysicalDeviceSelector selector { vkb_inst };
    vkb::PhysicalDevice physical_device = selector
        .set_minimum_version(1, 1)
        .set_surface(_surface)
        .select()
        .value();

    vkb::DeviceBuilder device_builder { physical_device };
    vkb::Device vkb_device = device_builder.build().value();

    _device = vkb_device.device;
    _gpu = physical_device.physical_device;

    _graphics_queue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    _graphics_queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();
}

void VulkanEngine::initSwapchain() {
    vkb::SwapchainBuilder swapchain_builder { _gpu, _device, _surface };
    vkb::Swapchain vkb_swapchain = swapchain_builder
        .use_default_format_selection()
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(_window_extent.width, _window_extent.height)
        .build()
        .value();

    _swapchain = vkb_swapchain.swapchain;
    _swapchain_images = vkb_swapchain.get_images().value();
    _swapchain_image_views = vkb_swapchain.get_image_views().value();
    _swapchain_image_format = vkb_swapchain.image_format;
}

void VulkanEngine::initCommands() {

   VkCommandPoolCreateInfo command_pool_info = vk_init::command_pool_create_info(_graphics_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
   errorCheck(vkCreateCommandPool(_device, &command_pool_info, nullptr, &_command_pool));

   VkCommandBufferAllocateInfo command_allocate_info = vk_init::command_buffer_allocate_info(_command_pool, 1);
   errorCheck(vkAllocateCommandBuffers(_device, &command_allocate_info, &_main_command_buffer));
}

void VulkanEngine::initRenderPass() {
    VkAttachmentDescription color_attachment {};
    color_attachment.format = _swapchain_image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkRenderPassCreateInfo render_pass_info {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;

    errorCheck(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_render_pass));
}

void VulkanEngine::initFramebuffers() {
    VkFramebufferCreateInfo framebuffer_info {};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.pNext = nullptr;
    framebuffer_info.renderPass = _render_pass;
    framebuffer_info.attachmentCount = 1;
    framebuffer_info.width = _window_extent.width;
    framebuffer_info.height = _window_extent.height;
    framebuffer_info.layers = 1;

    const uint32_t swapchain_image_count = _swapchain_images.size();
    _framebuffers = std::vector<VkFramebuffer>(swapchain_image_count);

    for (int x = 0; x < swapchain_image_count; x++) {
        framebuffer_info.pAttachments = &_swapchain_image_views[x];
        errorCheck(vkCreateFramebuffer(_device, &framebuffer_info, nullptr, &_framebuffers[x]));
    }
}

void VulkanEngine::initSyncStructures() {
    VkFenceCreateInfo fence_create_info {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = nullptr;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    errorCheck(vkCreateFence(_device, &fence_create_info, nullptr, &_render_fence));

    VkSemaphoreCreateInfo semaphore_create_info {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = nullptr;
    semaphore_create_info.flags = 0;

    errorCheck(vkCreateSemaphore(_device, &semaphore_create_info, nullptr, &_present_semaphore));
    errorCheck(vkCreateSemaphore(_device, &semaphore_create_info, nullptr, &_render_semaphore));
}


void VulkanEngine::cleanup() {
    if (_is_initialized) {
        vkDestroyCommandPool(_device, _command_pool, nullptr);
        vkDestroySwapchainKHR(_device, _swapchain, nullptr);
        vkDestroyRenderPass(_device, _render_pass, nullptr);
        
        for (int x = 0; x < _swapchain_image_views.size(); x++) {
            vkDestroyFramebuffer(_device, _framebuffers[x], nullptr);
            vkDestroyImageView(_device, _swapchain_image_views[x], nullptr);
        }

        vkDestroyDevice(_device, nullptr);
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
        vkDestroyInstance(_instance, nullptr);
        delete window;
        destroyPlatform();
    }
}

void VulkanEngine::draw() {
    errorCheck(vkWaitForFences(_device, 1, &_render_fence, true, 1000000000));
    errorCheck(vkResetFences(_device, 1, &_render_fence));

    uint32_t swapchain_image_index;
    errorCheck(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, _present_semaphore, nullptr, &swapchain_image_index));

    errorCheck(vkResetCommandBuffer(_main_command_buffer, 0));
    VkCommandBufferBeginInfo cmd_begin_info {};
    cmd_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_begin_info.pNext = nullptr;
    cmd_begin_info.pInheritanceInfo = nullptr;
    cmd_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    errorCheck(vkBeginCommandBuffer(_main_command_buffer, &cmd_begin_info));
    
    VkClearValue clear_value;
    float flash = abs(sin(_frame_number / 120.f));
    clear_value.color = {{ 0.0f, 0.0f, flash, 1.0f }};
    VkRenderPassBeginInfo render_pass_begin_info {};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = nullptr;
    render_pass_begin_info.renderPass = _render_pass;
    render_pass_begin_info.renderArea.offset.x = 0;
    render_pass_begin_info.renderArea.offset.y = 0;
    render_pass_begin_info.renderArea.extent = _window_extent;
    render_pass_begin_info.framebuffer = _framebuffers[swapchain_image_index];
    render_pass_begin_info.clearValueCount = 1;
    render_pass_begin_info.pClearValues = &clear_value;

    vkCmdBeginRenderPass(_main_command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(_main_command_buffer);

    errorCheck(vkEndCommandBuffer(_main_command_buffer));

    VkSubmitInfo submit {};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.pNext = nullptr;

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &_present_semaphore;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &_render_semaphore;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &_main_command_buffer;

    errorCheck(vkQueueSubmit(_graphics_queue, 1, &submit, _render_fence));

    VkPresentInfoKHR present_info {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.pSwapchains = &_swapchain;
    present_info.swapchainCount = 1;
    present_info.pWaitSemaphores = &_render_semaphore;
    present_info.waitSemaphoreCount = 1;
    present_info.pImageIndices = &swapchain_image_index;
    errorCheck(vkQueuePresentKHR(_graphics_queue, &present_info));

    _frame_number++;
}



void VulkanEngine::run() {
    bool run = true;

    while(run) {
        window->update();
        run = window->window_should_run;
        draw();
    }

}
