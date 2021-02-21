#define VMA_IMPLEMENTATION

#include "vk_engine.hpp"
#include "vk_types.hpp"
#include "vk_init.hpp"
#include "vk_textures.hpp"
#include "platform.hpp"

#include <math.h>

void VulkanEngine::init() {
    init_platform();
    init_vulkan();
    init_swapchain();
    init_render_pass();
    init_framebuffers();
    init_commands();
    init_sync_structures();
    init_descriptors();
    init_pipelines();
    load_meshes();
    load_images();
    init_scene();

    _is_initialized = true;
}

void VulkanEngine::init_vulkan() {
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

    VmaAllocatorCreateInfo allocator_info {};
    allocator_info.physicalDevice = _gpu;
    allocator_info.device = _device;
    allocator_info.instance = _instance;
    vmaCreateAllocator(&allocator_info, &_allocator);

    _main_deletion_queue.push_function(
            [=]() {
                vmaDestroyAllocator(_allocator);
            }
    );

    vkGetPhysicalDeviceProperties(_gpu, &_gpu_properties);
    //std::cout << "GPU has a minimum buffer alignment of " << _gpu_properties.limits.minUniformBufferOffsetAlignment << std::endl;
}


// TODO: Handle recreating swapchain
void VulkanEngine::init_swapchain() {

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

    _main_deletion_queue.push_function(
            [=]() {
                vkDestroySwapchainKHR(_device, _swapchain, nullptr);
            }
    );

    VkExtent3D depth_image_extent { _window_extent.width, _window_extent.height, 1 };
    _depth_format = VK_FORMAT_D32_SFLOAT;

    VkImageCreateInfo d_img_info = vk_init::image_create_info(_depth_format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depth_image_extent);

    VmaAllocationCreateInfo d_img_allocinfo {};
    d_img_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    d_img_allocinfo.requiredFlags = VkMemoryPropertyFlags (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vmaCreateImage(_allocator, &d_img_info, &d_img_allocinfo, &_depth_image._image, &_depth_image._allocation, nullptr);
	VkImageViewCreateInfo d_view_info = vk_init::image_view_create_info(_depth_format, _depth_image._image, VK_IMAGE_ASPECT_DEPTH_BIT);

    error_check(vkCreateImageView(_device, &d_view_info, nullptr, &_depth_image_view));

    _main_deletion_queue.push_function(
            [=]() {
                vkDestroyImageView(_device, _depth_image_view, nullptr);
                vmaDestroyImage(_allocator, _depth_image._image, _depth_image._allocation);
            }
    );
}

void VulkanEngine::init_commands() {

    VkCommandPoolCreateInfo command_pool_info = vk_init::command_pool_create_info(_graphics_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int x = 0; x < FRAME_OVERLAP; x++) {
        error_check(vkCreateCommandPool(_device, &command_pool_info, nullptr, &_frames[x]._command_pool));

        VkCommandBufferAllocateInfo command_allocate_info = vk_init::command_buffer_allocate_info(_frames[x]._command_pool, 1);
        error_check(vkAllocateCommandBuffers(_device, &command_allocate_info, &_frames[x]._main_command_buffer));

        //std::cout << "Initialized command buffer for frame: " << &_frames[x] << " with address: " << _frames[x]._main_command_buffer << std::endl;

        _main_deletion_queue.push_function(
                [=]() {
                     vkDestroyCommandPool(_device, _frames[x]._command_pool, nullptr);
                }
         );
    }

    VkCommandPoolCreateInfo upload_command_pool_info = vk_init::command_pool_create_info(_graphics_queue_family);
    error_check(vkCreateCommandPool(_device, &upload_command_pool_info, nullptr, &_upload_context._command_pool));

    _main_deletion_queue.push_function(
            [=]() {
                vkDestroyCommandPool(_device, _upload_context._command_pool, nullptr);
            }
    );
}

void VulkanEngine::init_render_pass() {
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

    VkAttachmentDescription depth_attachment {};
    depth_attachment.flags = 0;
    depth_attachment.format = _depth_format;
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref {};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    VkSubpassDependency dependency {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachments[2] = { color_attachment, depth_attachment };

    VkRenderPassCreateInfo render_pass_info {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 2;
    render_pass_info.pAttachments = attachments;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    error_check(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_render_pass));

    _main_deletion_queue.push_function(
            [=]() {
                vkDestroyRenderPass(_device, _render_pass, nullptr);
            }
    );
}

void VulkanEngine::init_framebuffers() {
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
        VkImageView attachments[2];
        attachments[0] = _swapchain_image_views[x];
        attachments[1] = _depth_image_view;

        framebuffer_info.pAttachments = attachments;
        framebuffer_info.attachmentCount = 2;

        error_check(vkCreateFramebuffer(_device, &framebuffer_info, nullptr, &_framebuffers[x]));

        _main_deletion_queue.push_function(
                [=]() {
                    vkDestroyFramebuffer(_device, _framebuffers[x], nullptr);
                    vkDestroyImageView(_device, _swapchain_image_views[x], nullptr);
                }
        );
    }
}

void VulkanEngine::init_sync_structures() {

    VkFenceCreateInfo fence_create_info {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = nullptr;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkFenceCreateInfo upload_fence_create_info {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = nullptr;


    error_check(vkCreateFence(_device, &upload_fence_create_info, nullptr, &_upload_context._upload_fence));
    _main_deletion_queue.push_function(
            [=]() {
                vkDestroyFence(_device, _upload_context._upload_fence, nullptr);
            }
    );

    VkSemaphoreCreateInfo semaphore_create_info {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = nullptr;
    semaphore_create_info.flags = 0;

    for (int x = 0; x < FRAME_OVERLAP; x++) {
        error_check(vkCreateFence(_device, &fence_create_info, nullptr, &_frames[x]._render_fence));


        error_check(vkCreateSemaphore(_device, &semaphore_create_info, nullptr, &_frames[x]._present_semaphore));
        error_check(vkCreateSemaphore(_device, &semaphore_create_info, nullptr, &_frames[x]._render_semaphore));

        _main_deletion_queue.push_function(
                [=]() {
                    vkDestroySemaphore(_device, _frames[x]._present_semaphore, nullptr);
                    vkDestroySemaphore(_device, _frames[x]._render_semaphore, nullptr);
                    vkDestroyFence(_device, _frames[x]._render_fence, nullptr);
                }
        );
    }
}

void VulkanEngine::init_descriptors() {


    std::vector<VkDescriptorPoolSize> sizes = { 
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 }
    };

    VkDescriptorSetLayoutBinding object_bind = vk_init::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);

    VkDescriptorSetLayoutCreateInfo set_to_info {};
    set_to_info.bindingCount = 1;
    set_to_info.flags = 0;
    set_to_info.pNext = nullptr;
    set_to_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    set_to_info.pBindings = &object_bind;

    vkCreateDescriptorSetLayout(_device, &set_to_info, nullptr, &_object_set_layout);

    VkDescriptorPoolCreateInfo pool_info {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = 0;
    pool_info.maxSets = 10;
    pool_info.poolSizeCount = (uint32_t) sizes.size();
    pool_info.pPoolSizes = sizes.data();

    vkCreateDescriptorPool(_device, &pool_info, nullptr, &_descriptor_pool);

    const size_t scene_param_buffer_size = FRAME_OVERLAP * pad_uniform_buffer_size(sizeof(GPUSceneData));
    _scene_parameter_buffer = create_buffer(scene_param_buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    
    VkDescriptorSetLayoutBinding camera_buffer_binding = vk_init::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
    VkDescriptorSetLayoutBinding scene_buffer_binding = vk_init::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);

    VkDescriptorSetLayoutBinding bindings[] = { camera_buffer_binding, scene_buffer_binding };
    
    VkDescriptorSetLayoutCreateInfo set_info {};
    set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    set_info.pNext = nullptr;
    set_info.bindingCount = 2;
    set_info.flags = 0;
    set_info.pBindings = bindings;

    vkCreateDescriptorSetLayout(_device, &set_info, nullptr, &_global_set_layout);

    VkDescriptorSetLayoutBinding texture_binding = vk_init::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);

    VkDescriptorSetLayoutCreateInfo set_3_info {};
    set_3_info.bindingCount = 1;
    set_3_info.flags = 0;
    set_3_info.pNext = nullptr;
    set_3_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    set_3_info.pBindings = &texture_binding;

    vkCreateDescriptorSetLayout(_device, &set_3_info, nullptr, &_single_texture_set_layout);
    
    const int MAX_OBJECTS = 10000;
    for (int x = 0; x < FRAME_OVERLAP; x++) {
        _frames[x].camera_buffer = create_buffer(sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        _frames[x].object_buffer = create_buffer(sizeof(GPUObjectData) * MAX_OBJECTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        VkDescriptorSetAllocateInfo allocate_info {};
        allocate_info.pNext = nullptr;
        allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocate_info.descriptorPool = _descriptor_pool;
        allocate_info.descriptorSetCount = 1;
        allocate_info.pSetLayouts = &_global_set_layout;

        vkAllocateDescriptorSets(_device, &allocate_info, &_frames[x].global_descriptor);

        VkDescriptorSetAllocateInfo object_set_alloc {};
        object_set_alloc.pNext = nullptr;
        object_set_alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        object_set_alloc.descriptorPool = _descriptor_pool;
        object_set_alloc.descriptorSetCount = 1;
        object_set_alloc.pSetLayouts = &_object_set_layout;

        vkAllocateDescriptorSets(_device, &object_set_alloc, &_frames[x].object_descriptor);

        VkDescriptorBufferInfo camera_info;
        camera_info.buffer = _frames[x].camera_buffer._buffer;
        camera_info.offset = 0;
        camera_info.range = sizeof(GPUCameraData);

        VkDescriptorBufferInfo scene_info;
        scene_info.buffer = _scene_parameter_buffer._buffer;
        scene_info.offset = 0;
        scene_info.range = sizeof(GPUSceneData);

        VkDescriptorBufferInfo object_buffer_info {};
        object_buffer_info.buffer = _frames[x].object_buffer._buffer;
        object_buffer_info.offset = 0;
        object_buffer_info.range = sizeof(GPUObjectData) * MAX_OBJECTS;

        VkWriteDescriptorSet camera_write = vk_init::write_descriptor_buffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, _frames[x].global_descriptor, &camera_info, 0);
        VkWriteDescriptorSet scene_write = vk_init::write_descriptor_buffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, _frames[x].global_descriptor, &scene_info, 1);
        VkWriteDescriptorSet object_write = vk_init::write_descriptor_buffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, _frames[x].object_descriptor, &object_buffer_info, 0);
        VkWriteDescriptorSet set_writes[] = { camera_write, scene_write, object_write };

        vkUpdateDescriptorSets(_device, 3, set_writes, 0, nullptr);
    }
}

void VulkanEngine::init_scene() {

    RenderObject empire;

    empire.mesh = get_mesh("empire");
    empire.material = get_material("textured_mesh");
    empire.transform_matrix = glm::translate(glm::vec3 {5, -10, 0});

    _renderables.push_back(empire);

    
    Material* textured_mat = get_material("textured_mesh");

    VkDescriptorSetAllocateInfo alloc_info {};
    alloc_info.pNext = nullptr;
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = _descriptor_pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts - &_single_texture_set_layout;

    vkAllocateDescriptorSets(_device, &alloc_info, &textured_mat->texture_set);

    VkSamplerCreateInfo sampler_info = vk_init::sampler_create_info(VK_FILTER_NEAREST);
    VkSampler blocky_sampler;
    vkCreateSampler(_device, &sampler_info, nullptr, &blocky_sampler);

    _main_deletion_queue.push_function(
            [=]() {
                vkDestroySampler(_device, blocky_sampler, nullptr);
            }
    );

    VkDescriptorImageInfo image_buffer_info;
    image_buffer_info.sampler = blocky_sampler;
    image_buffer_info.imageView = _loaded_textures["empire_diffuse"].image_view;
    image_buffer_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet texture = vk_init::write_descriptor_image(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, textured_mat->texture_set, &image_buffer_info, 0);

    vkUpdateDescriptorSets(_device, 1, &texture, 0, nullptr);
}


void VulkanEngine::cleanup() {
    if (_is_initialized) {
        for (int x = 0; x < FRAME_OVERLAP; x++) {
            vkWaitForFences(_device, 1, &_frames[x]._render_fence, true, 1000000000);
        }

        _main_deletion_queue.flush();

        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        vkDestroyDevice(_device, nullptr);

        vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);

        vkDestroyInstance(_instance, nullptr);

        delete window;
        destroy_platform();
    }
}

void VulkanEngine::draw() {
    FrameData frame = get_current_frame();
    //std::cout << "Current frame: " << _frame_number % FRAME_OVERLAP << std::endl;
    error_check(vkWaitForFences(_device, 1, &get_current_frame()._render_fence, true, 1000000000));
    error_check(vkResetFences(_device, 1, &get_current_frame()._render_fence));
    error_check(vkResetCommandBuffer(get_current_frame()._main_command_buffer, 0));

    uint32_t swapchain_image_index;
    error_check(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, get_current_frame()._present_semaphore, nullptr, &swapchain_image_index));

    VkCommandBufferBeginInfo cmd_begin_info {};
    cmd_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_begin_info.pNext = nullptr;
    cmd_begin_info.pInheritanceInfo = nullptr;
    cmd_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    error_check(vkBeginCommandBuffer(get_current_frame()._main_command_buffer, &cmd_begin_info));

    VkRenderPassBeginInfo render_pass_begin_info {};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = nullptr;
    render_pass_begin_info.renderPass = _render_pass;
    render_pass_begin_info.renderArea.offset.x = 0;
    render_pass_begin_info.renderArea.offset.y = 0;
    render_pass_begin_info.renderArea.extent = _window_extent;
    render_pass_begin_info.framebuffer = _framebuffers[swapchain_image_index];
    render_pass_begin_info.clearValueCount = 2;
    
    VkClearValue clear_value;
    float flash = abs(sin(_frame_number / 120.f));
    clear_value.color = {{ 0.0f, 0.0f, flash, 1.0f }};
    VkClearValue depth_clear;
    depth_clear.depthStencil.depth = 1.f;
    VkClearValue clear_values[] = { clear_value, depth_clear };

    render_pass_begin_info.pClearValues = clear_values;
     
    vkCmdBeginRenderPass(get_current_frame()._main_command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    //std::cout << "Drawing command buffer for frame: " << &get_current_frame() << " with address: " << get_current_frame()._main_command_buffer << std::endl;
    draw_objects(get_current_frame()._main_command_buffer, _renderables.data(), _renderables.size());
    
    vkCmdEndRenderPass(get_current_frame()._main_command_buffer);

    error_check(vkEndCommandBuffer(get_current_frame()._main_command_buffer));

    VkSubmitInfo submit {};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.pNext = nullptr;

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &get_current_frame()._present_semaphore;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &get_current_frame()._render_semaphore;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &get_current_frame()._main_command_buffer;

    error_check(vkQueueSubmit(_graphics_queue, 1, &submit, get_current_frame()._render_fence));

    VkPresentInfoKHR present_info {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.pSwapchains = &_swapchain;
    present_info.swapchainCount = 1;
    present_info.pWaitSemaphores = &get_current_frame()._render_semaphore;
    present_info.waitSemaphoreCount = 1;
    present_info.pImageIndices = &swapchain_image_index;

    error_check(vkQueuePresentKHR(_graphics_queue, &present_info));

    _frame_number++;
}

void VulkanEngine::init_pipelines() {
    // <++> tmp
    VkShaderModule triangle_frag_shader;
    if (!load_shader_module("./shaders/triangle.frag.spv", &triangle_frag_shader)) {
        std::cout << "Error when building the triangle fragment shader module" << std::endl;
    } else {
        std::cout << "Triangle fragment shader successfully loaded" << std::endl;
    }

    VkShaderModule triangle_vert_shader;
    if (!load_shader_module("./shaders/triangle.vert.spv", &triangle_vert_shader)) {
        std::cout << "Error when building the triangle vertex shader module" << std::endl;
    } else {
        std::cout << "Triangle vertex shader successfully loaded" << std::endl;
    }

    VkPipelineLayoutCreateInfo pipeline_layout_info = vk_init::pipeline_layout_create_info();
    
    VkPushConstantRange push_constant;
    push_constant.offset = 0;
    push_constant.size = sizeof(MeshPushConstants);

    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pipeline_layout_info.pPushConstantRanges = &push_constant;
    pipeline_layout_info.pushConstantRangeCount = 1;
    
    VkDescriptorSetLayout set_layouts[] = { _global_set_layout, _object_set_layout };
    pipeline_layout_info.setLayoutCount = 2;
    pipeline_layout_info.pSetLayouts = set_layouts;

    error_check(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_triangle_pipeline_layout));

    PipelineBuilder pipeline_builder;

    pipeline_builder._vertex_input_info = vk_init::vertex_input_state_create_info();

    pipeline_builder._input_assembly = vk_init::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    pipeline_builder._viewport.x = 0.0f;
    pipeline_builder._viewport.y = 0.0f;
    pipeline_builder._viewport.width = (float)_window_extent.width;
    pipeline_builder._viewport.height = (float)_window_extent.height;
    pipeline_builder._viewport.minDepth = 0.0f;
    pipeline_builder._viewport.maxDepth = 1.0f;

    pipeline_builder._scissor.offset = { 0, 0 };
    pipeline_builder._scissor.extent = _window_extent;

    pipeline_builder._rasterizer = vk_init::rasterization_state_create_info(VK_POLYGON_MODE_FILL);

    pipeline_builder._multisampling = vk_init::multisample_state_create_info();

    pipeline_builder._color_blend_attachment = vk_init::color_blend_attachment_state();

    pipeline_builder._pipeline_layout = _triangle_pipeline_layout;

    pipeline_builder._depth_stencil = vk_init::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

    VertexInputDescription vertex_description = Vertex::get_vertex_description();
    pipeline_builder._vertex_input_info.pVertexAttributeDescriptions = vertex_description.attributes.data();
    pipeline_builder._vertex_input_info.vertexAttributeDescriptionCount = vertex_description.attributes.size();
    pipeline_builder._vertex_input_info.pVertexBindingDescriptions = vertex_description.bindings.data();
    pipeline_builder._vertex_input_info.vertexBindingDescriptionCount = vertex_description.bindings.size();

    pipeline_builder._shader_stages.push_back(vk_init::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, triangle_vert_shader));
    pipeline_builder._shader_stages.push_back(vk_init::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, triangle_frag_shader));

    _triangle_pipeline = pipeline_builder.build_pipeline(_device, _render_pass);
    create_material(_triangle_pipeline, _triangle_pipeline_layout, "textured_mesh");

    vkDestroyShaderModule(_device, triangle_frag_shader, nullptr);
    vkDestroyShaderModule(_device, triangle_vert_shader, nullptr);

    _main_deletion_queue.push_function(
            [=]() {
                vkDestroyPipeline(_device, _triangle_pipeline, nullptr);
                vkDestroyPipelineLayout(_device, _triangle_pipeline_layout, nullptr);
            }
    );
    // <++>
}

void VulkanEngine::run() {
    bool run = true;
    while(run) {
        window->update();
        run = window->window_should_run;
        draw();
    }

}


bool VulkanEngine::load_shader_module(const char* file_path, VkShaderModule* out_shader_module) {
    std::ifstream file(file_path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        return false;
    }

    size_t file_size = (size_t) file.tellg();
    
    std::vector<uint32_t> buffer (file_size / sizeof(uint32_t));
    file.seekg(0);
    file.read((char*) buffer.data(), file_size);
    file.close();

    VkShaderModuleCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.codeSize = buffer.size() * sizeof(uint32_t);
    create_info.pCode = buffer.data();

    VkShaderModule shader_module;
    if (vkCreateShaderModule(_device, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
        return false;
    }

    *out_shader_module = shader_module;
    return true;
}

void VulkanEngine::load_meshes() {
    // <++> tmp
    
    Mesh lost_empire {};
    Mesh monkey_mesh {};

    lost_empire.load_from_obj("assets/lost_empire.obj", "assets/");
    monkey_mesh.load_from_obj("assets/monkey_smooth.obj", "assets/");

    upload_mesh(monkey_mesh);
    upload_mesh(lost_empire);

    _meshes["monkey"] = monkey_mesh;
    _meshes["empire"] = lost_empire;
    // <++>
}

void VulkanEngine::upload_mesh(Mesh& mesh) {
    const size_t buffer_size = mesh._vertices.size() * sizeof(Vertex);

    // Staging Buffer
    VkBufferCreateInfo staging_buffer_info {};
    staging_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    staging_buffer_info.size = buffer_size;
    staging_buffer_info.pNext = nullptr;
    staging_buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    VmaAllocationCreateInfo vma_alloc_info {};
    vma_alloc_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;

    AllocatedBuffer staging_buffer;

    error_check(vmaCreateBuffer(_allocator, 
                &staging_buffer_info, 
                &vma_alloc_info, 
                &staging_buffer._buffer, 
                &staging_buffer._allocation, 
                nullptr));

    void* data;
    vmaMapMemory(_allocator, staging_buffer._allocation, &data);
    memcpy(data, mesh._vertices.data(), buffer_size);
    vmaUnmapMemory(_allocator, staging_buffer._allocation);
    
    // Vertex Buffer
    VkBufferCreateInfo vertex_buffer_info {};
    vertex_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertex_buffer_info.pNext = nullptr;
    vertex_buffer_info.size = buffer_size;
    vertex_buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    vma_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    error_check(vmaCreateBuffer(_allocator, 
                &vertex_buffer_info, 
                &vma_alloc_info, 
                &mesh._vertex_buffer._buffer, 
                &mesh._vertex_buffer._allocation, 
                nullptr));
    
    // Copy
    immediate_submit(
            [=](VkCommandBuffer cmd) {
                VkBufferCopy copy;
                copy.dstOffset = 0;
                copy.srcOffset = 0;
                copy.size = buffer_size;
                vkCmdCopyBuffer(cmd, staging_buffer._buffer, mesh._vertex_buffer._buffer, 1, &copy);
            }
    );

    _main_deletion_queue.push_function(
            [=]() {
                vmaDestroyBuffer(_allocator, mesh._vertex_buffer._buffer, mesh._vertex_buffer._allocation);
            }
    );

    vmaDestroyBuffer(_allocator, staging_buffer._buffer, staging_buffer._allocation);

}

VkPipeline PipelineBuilder::build_pipeline(VkDevice device, VkRenderPass pass) {
    VkPipelineViewportStateCreateInfo viewport_state {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.pNext = nullptr;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &_viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &_scissor;

    VkPipelineColorBlendStateCreateInfo color_blending {};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.pNext = nullptr;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &_color_blend_attachment;

    VkGraphicsPipelineCreateInfo pipeline_info {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.pNext = nullptr;
    pipeline_info.stageCount = _shader_stages.size();
    pipeline_info.pStages = _shader_stages.data();
    pipeline_info.pVertexInputState = &_vertex_input_info;
    pipeline_info.pInputAssemblyState = &_input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &_rasterizer;
    pipeline_info.pMultisampleState = &_multisampling;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDepthStencilState = &_depth_stencil;
    pipeline_info.layout = _pipeline_layout;
    pipeline_info.renderPass = pass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

    VkPipeline new_pipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &new_pipeline) != VK_SUCCESS) {
        std::cout << "failed to create pipeline" << std::endl;
        return VK_NULL_HANDLE;
    } else {
        return new_pipeline;
    }
}

void VulkanEngine::draw_objects(VkCommandBuffer cmd, RenderObject* objects, int count) {
    glm::vec3 cam_pos = { 0.f, -3.f, -10.f };
    glm::mat4 view = glm::translate(glm::mat4(1.f), cam_pos);
    glm::mat4 projection = glm::perspective(glm::radians(70.f), 1700.f / 900.f, 0.1f, 200.f);
    projection[1][1] *= -1;

    GPUCameraData cam_data;
    cam_data.projection = projection;
    cam_data.view = view;
    cam_data.viewproj = projection * view;

    void* data;
    vmaMapMemory(_allocator, get_current_frame().camera_buffer._allocation, &data);
    memcpy(data, &cam_data, sizeof(GPUCameraData));
    vmaUnmapMemory(_allocator, get_current_frame().camera_buffer._allocation);

    void* object_data;
    vmaMapMemory(_allocator, get_current_frame().object_buffer._allocation, &object_data);
    GPUObjectData* objectSSBO = (GPUObjectData*) object_data;

    for (int x = 0; x < count; x++) {
        RenderObject& object = objects[x];
        objectSSBO[x].model_matrix = object.transform_matrix;
    }

    vmaUnmapMemory(_allocator, get_current_frame().object_buffer._allocation);

    float framed = (_frame_number / 120.f);
    _scene_parameters.ambient_color = { sin(framed), 0, cos(framed), 1};

    char* scene_data;
    vmaMapMemory(_allocator, _scene_parameter_buffer._allocation, (void**) &scene_data);
    int frame_index = _frame_number % FRAME_OVERLAP;
    scene_data += pad_uniform_buffer_size(sizeof(GPUSceneData)) * frame_index;
    memcpy(scene_data, &_scene_parameters, sizeof(GPUSceneData));
    vmaUnmapMemory(_allocator, _scene_parameter_buffer._allocation);

    Mesh* last_mesh = nullptr;
    Material* last_material = nullptr;

    for (int x = 0; x < count; x++) {
        RenderObject& object = objects[x];
        if (object.material != last_material) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
            last_material = object.material;

            uint32_t uniform_offset = pad_uniform_buffer_size(sizeof(GPUSceneData)) * frame_index;
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline_layout,
                    0, 1, &get_current_frame().global_descriptor, 1, &uniform_offset);

            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline_layout,
                    1, 1, &get_current_frame().object_descriptor, 0, nullptr);
        }

        glm::mat4 model = object.transform_matrix;
        glm::mat4 mesh_matrix = projection * view * model;

        MeshPushConstants constants;
        constants.render_matrix = object.transform_matrix;
        vkCmdPushConstants(cmd, object.material->pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);
        if (object.mesh != last_mesh) {
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &object.mesh->_vertex_buffer._buffer, &offset);
            last_mesh = object.mesh;

            if (object.material->texture_set != VK_NULL_HANDLE) {
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline_layout, 2, 1, &object.material->texture_set, 0, nullptr);
            }
        }

        vkCmdDraw(cmd, object.mesh->_vertices.size(), 1, 0, 0);
    }
}

void VulkanEngine::immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function) {
    VkCommandBufferAllocateInfo command_allocate_info = vk_init::command_buffer_allocate_info(_upload_context._command_pool, 1);

    VkCommandBuffer cmd;
    error_check(vkAllocateCommandBuffers(_device, &command_allocate_info, &cmd));

    VkCommandBufferBeginInfo cmd_begin_info {};
    cmd_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_begin_info.pNext = nullptr;
    cmd_begin_info.pInheritanceInfo = nullptr;
    cmd_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    error_check(vkBeginCommandBuffer(cmd, &cmd_begin_info));

    function(cmd);

    error_check(vkEndCommandBuffer(cmd));

    VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = nullptr;
	submit_info.waitSemaphoreCount = 0;
	submit_info.pWaitSemaphores = nullptr;
	submit_info.pWaitDstStageMask = nullptr;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd;
	submit_info.signalSemaphoreCount = 0;
	submit_info.pSignalSemaphores = nullptr;
    
    error_check(vkQueueSubmit(_graphics_queue, 1, &submit_info, _upload_context._upload_fence));
    vkWaitForFences(_device, 1, &_upload_context._upload_fence, true, 99999999999);
    vkResetFences(_device, 1, &_upload_context._upload_fence);

    vkResetCommandPool(_device, _upload_context._command_pool, 0);

}

// https://github.com/SaschaWillems/Vulkan/tree/master/examples/dynamicuniformbuffer
size_t VulkanEngine::pad_uniform_buffer_size(size_t original_size) {
    size_t min_ubo_alignment = _gpu_properties.limits.minUniformBufferOffsetAlignment;
    size_t aligned_size = original_size;

    if (min_ubo_alignment > 0) {
        aligned_size = (aligned_size + min_ubo_alignment - 1) & ~(min_ubo_alignment - 1);
    }

    return aligned_size;
}

void VulkanEngine::load_images() {
    Texture lost_empire;
    vk_util::load_image_from_file(*this, "./assets/lost_empire-RGBA.png", lost_empire.image);
    VkImageViewCreateInfo image_info = vk_init::image_view_create_info(VK_FORMAT_R8G8B8A8_UNORM, lost_empire.image._image, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCreateImageView(_device, &image_info, nullptr, &lost_empire.image_view);
    _loaded_textures["empire_diffuse"] = lost_empire;
}

AllocatedBuffer VulkanEngine::create_buffer(size_t alloc_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage) {
    VkBufferCreateInfo buffer_info {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.pNext = nullptr;
    buffer_info.size = alloc_size;
    buffer_info.usage = usage;

    VmaAllocationCreateInfo vma_alloc_info {};
    vma_alloc_info.usage = memory_usage;

    AllocatedBuffer new_buffer;

    error_check(vmaCreateBuffer(_allocator, &buffer_info, &vma_alloc_info, &new_buffer._buffer, &new_buffer._allocation, nullptr));

    _main_deletion_queue.push_function(
            [=]() {
                vmaDestroyBuffer(_allocator, new_buffer._buffer, new_buffer._allocation);
            }
    );

    return new_buffer;
}

Material* VulkanEngine::create_material(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name) {
    Material mat;
    mat.pipeline = pipeline;
    mat.pipeline_layout = layout;
    _materials[name] = mat;
    return &_materials[name];
}

Material* VulkanEngine::get_material(const std::string& name) {
    auto it = _materials.find(name);
    if (it == _materials.end()) {
        return nullptr;
    }

    return &(*it).second;
}

Mesh* VulkanEngine::get_mesh(const std::string& name) {
    auto it = _meshes.find(name);
    if (it == _meshes.end()) {
        return nullptr;
    }

    return &(*it).second;
}

FrameData& VulkanEngine::get_current_frame() {
    return _frames[_frame_number % FRAME_OVERLAP];
}



