#define VMA_IMPLEMENTATION

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
    initPipelines();
    loadMeshes();
    initScene();

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

    _main_deletion_queue.push_function(
            [=]() {
                vkDestroySwapchainKHR(_device, _swapchain, nullptr);
            }
    );

    VkExtent3D depth_image_extent { _window_extent.width, _window_extent.height, 1 };
    _depth_format = VK_FORMAT_D32_SFLOAT;

    VkImageCreateInfo d_img_info = vk_init::imageCreateInfo(_depth_format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depth_image_extent);

    VmaAllocationCreateInfo d_img_allocinfo {};
    d_img_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    d_img_allocinfo.requiredFlags = VkMemoryPropertyFlags (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vmaCreateImage(_allocator, &d_img_info, &d_img_allocinfo, &_depth_image._image, &_depth_image._allocation, nullptr);
	VkImageViewCreateInfo d_view_info = vk_init::imageViewCreateInfo(_depth_format, _depth_image._image, VK_IMAGE_ASPECT_DEPTH_BIT);

    errorCheck(vkCreateImageView(_device, &d_view_info, nullptr, &_depth_image_view));

    _main_deletion_queue.push_function(
            [=]() {
                vkDestroyImageView(_device, _depth_image_view, nullptr);
                vmaDestroyImage(_allocator, _depth_image._image, _depth_image._allocation);
            }
    );
}

void VulkanEngine::initCommands() {

    VkCommandPoolCreateInfo command_pool_info = vk_init::commandPoolCreateInfo(_graphics_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int x = 0; x < FRAME_OVERLAP; x++) {
        errorCheck(vkCreateCommandPool(_device, &command_pool_info, nullptr, &_frames[x]._command_pool));

        VkCommandBufferAllocateInfo command_allocate_info = vk_init::commandBufferAllocateInfo(_frames[x]._command_pool, 1);
        errorCheck(vkAllocateCommandBuffers(_device, &command_allocate_info, &_frames[x]._main_command_buffer));

        _main_deletion_queue.push_function(
                [=]() {
                     vkDestroyCommandPool(_device, _frames[x]._command_pool, nullptr);
                }
         );
    }
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

    VkAttachmentDescription attachments[2] = { color_attachment, depth_attachment };

    VkRenderPassCreateInfo render_pass_info {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 2;
    render_pass_info.pAttachments = attachments;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;

    errorCheck(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_render_pass));

    _main_deletion_queue.push_function(
            [=]() {
                vkDestroyRenderPass(_device, _render_pass, nullptr);
            }
    );
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
        VkImageView attachments[2];
        attachments[0] = _swapchain_image_views[x];
        attachments[1] = _depth_image_view;

        framebuffer_info.pAttachments = attachments;
        framebuffer_info.attachmentCount = 2;

        errorCheck(vkCreateFramebuffer(_device, &framebuffer_info, nullptr, &_framebuffers[x]));

        _main_deletion_queue.push_function(
                [=]() {
                    vkDestroyFramebuffer(_device, _framebuffers[x], nullptr);
                    vkDestroyImageView(_device, _swapchain_image_views[x], nullptr);
                }
        );
    }
}

void VulkanEngine::initSyncStructures() {

    VkFenceCreateInfo fence_create_info {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = nullptr;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo semaphore_create_info {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = nullptr;
    semaphore_create_info.flags = 0;

    for (int x = 0; x < FRAME_OVERLAP; x++) {
        errorCheck(vkCreateFence(_device, &fence_create_info, nullptr, &_frames[x]._render_fence));


        errorCheck(vkCreateSemaphore(_device, &semaphore_create_info, nullptr, &_frames[x]._present_semaphore));
        errorCheck(vkCreateSemaphore(_device, &semaphore_create_info, nullptr, &_frames[x]._render_semaphore));

        _main_deletion_queue.push_function(
                [=]() {
                    vkDestroySemaphore(_device, _frames[x]._present_semaphore, nullptr);
                    vkDestroySemaphore(_device, _frames[x]._render_semaphore, nullptr);
                }
        );
    }
}

void VulkanEngine::initScene() {
    RenderObject monkey;
    monkey.mesh = getMesh("monkey");
    monkey.material = getMaterial("default_mesh");
    monkey.transform_matrix = glm::mat4 { 1.0f };

    _renderables.push_back(monkey);

    for (int x = -20; x < 20; x++) {
        for (int y = -20; y < 20; y++) {
            RenderObject monkey;
            monkey.mesh = getMesh("monkey");
            monkey.material = getMaterial("default_mesh");
            glm::mat4 translation = glm::translate(glm::mat4 { 1.0 }, glm::vec3(x, 0, y));
            glm::mat4 scale = glm::scale(glm::mat4 { 1.0 }, glm::vec3(0.2, 0.2, 0.2));
            monkey.transform_matrix = translation * scale;
            _renderables.push_back(monkey);
        }
    }
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
        destroyPlatform();
    }
}

void VulkanEngine::draw() {
    FrameData frame = getCurrentFrame();
    errorCheck(vkWaitForFences(_device, 1, &frame._render_fence, true, 1000000000));
    errorCheck(vkResetFences(_device, 1, &frame._render_fence));

    errorCheck(vkResetCommandBuffer(frame._main_command_buffer, 0));

    uint32_t swapchain_image_index;
    errorCheck(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, frame._present_semaphore, nullptr, &swapchain_image_index));

    VkCommandBufferBeginInfo cmd_begin_info {};
    cmd_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_begin_info.pNext = nullptr;
    cmd_begin_info.pInheritanceInfo = nullptr;
    cmd_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    errorCheck(vkBeginCommandBuffer(frame._main_command_buffer, &cmd_begin_info));
    
    VkClearValue clear_value;
    float flash = abs(sin(_frame_number / 120.f));
    clear_value.color = {{ 0.0f, 0.0f, flash, 1.0f }};

    VkClearValue depth_clear;
    depth_clear.depthStencil.depth = 1.f;

    VkRenderPassBeginInfo render_pass_begin_info {};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = nullptr;
    render_pass_begin_info.renderPass = _render_pass;
    render_pass_begin_info.renderArea.offset.x = 0;
    render_pass_begin_info.renderArea.offset.y = 0;
    render_pass_begin_info.renderArea.extent = _window_extent;
    render_pass_begin_info.framebuffer = _framebuffers[swapchain_image_index];
    render_pass_begin_info.clearValueCount = 2;

    VkClearValue clear_values[] = { clear_value, depth_clear };
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(frame._main_command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    drawObjects(frame._main_command_buffer, _renderables.data(), _renderables.size());
    
    vkCmdEndRenderPass(frame._main_command_buffer);

    errorCheck(vkEndCommandBuffer(frame._main_command_buffer));

    VkSubmitInfo submit {};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.pNext = nullptr;

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &frame._present_semaphore;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &frame._render_semaphore;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &frame._main_command_buffer;

    errorCheck(vkQueueSubmit(_graphics_queue, 1, &submit, frame._render_fence));

    VkPresentInfoKHR present_info {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.pSwapchains = &_swapchain;
    present_info.swapchainCount = 1;
    present_info.pWaitSemaphores = &frame._render_semaphore;
    present_info.waitSemaphoreCount = 1;
    present_info.pImageIndices = &swapchain_image_index;

    errorCheck(vkQueuePresentKHR(_graphics_queue, &present_info));

    _frame_number++;
}

void VulkanEngine::initPipelines() {
    // <++> tmp
    VkShaderModule triangle_frag_shader;
    if (!loadShaderModule("./shaders/triangle.frag.spv", &triangle_frag_shader)) {
        std::cout << "Error when building the triangle fragment shader module" << std::endl;
    } else {
        std::cout << "Triangle fragment shader successfully loaded" << std::endl;
    }

    VkShaderModule triangle_vert_shader;
    if (!loadShaderModule("./shaders/triangle.vert.spv", &triangle_vert_shader)) {
        std::cout << "Error when building the triangle vertex shader module" << std::endl;
    } else {
        std::cout << "Triangle vertex shader successfully loaded" << std::endl;
    }

    VkPipelineLayoutCreateInfo pipeline_layout_info = vk_init::pipelineLayoutCreateInfo();
    
    VkPushConstantRange push_constant;
    push_constant.offset = 0;
    push_constant.size = sizeof(MeshPushConstants);

    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pipeline_layout_info.pPushConstantRanges = &push_constant;
    pipeline_layout_info.pushConstantRangeCount = 1;

    errorCheck(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_triangle_pipeline_layout));

    PipelineBuilder pipeline_builder;

    pipeline_builder._vertex_input_info = vk_init::vertexInputStateCreateInfo();

    pipeline_builder._input_assembly = vk_init::inputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    pipeline_builder._viewport.x = 0.0f;
    pipeline_builder._viewport.y = 0.0f;
    pipeline_builder._viewport.width = (float)_window_extent.width;
    pipeline_builder._viewport.height = (float)_window_extent.height;
    pipeline_builder._viewport.minDepth = 0.0f;
    pipeline_builder._viewport.maxDepth = 1.0f;

    pipeline_builder._scissor.offset = { 0, 0 };
    pipeline_builder._scissor.extent = _window_extent;

    pipeline_builder._rasterizer = vk_init::rasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);

    pipeline_builder._multisampling = vk_init::multisampleStateCreateInfo();

    pipeline_builder._color_blend_attachment = vk_init::colorBlendAttachmentState();

    pipeline_builder._pipeline_layout = _triangle_pipeline_layout;

    pipeline_builder._depth_stencil = vk_init::depthStencilCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

    VertexInputDescription vertex_description = Vertex::getVertexDescription();
    pipeline_builder._vertex_input_info.pVertexAttributeDescriptions = vertex_description.attributes.data();
    pipeline_builder._vertex_input_info.vertexAttributeDescriptionCount = vertex_description.attributes.size();
    pipeline_builder._vertex_input_info.pVertexBindingDescriptions = vertex_description.bindings.data();
    pipeline_builder._vertex_input_info.vertexBindingDescriptionCount = vertex_description.bindings.size();

    pipeline_builder._shader_stages.push_back(vk_init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, triangle_vert_shader));
    pipeline_builder._shader_stages.push_back(vk_init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, triangle_frag_shader));

    _triangle_pipeline = pipeline_builder.buildPipeline(_device, _render_pass);
    createMaterial(_triangle_pipeline, _triangle_pipeline_layout, "default_mesh");

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


bool VulkanEngine::loadShaderModule(const char* file_path, VkShaderModule* out_shader_module) {
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

void VulkanEngine::loadMeshes() {
    // <++> tmp
    _triangle_mesh._vertices.resize(3);
    _triangle_mesh._vertices[0].position = { 1.f, 1.f, 0.0f };
    _triangle_mesh._vertices[1].position = { -1.f, 1.f, 0.0f };
    _triangle_mesh._vertices[2].position = { 0.f, -1.f, 0.0f };

    _triangle_mesh._vertices[0].color = { 0.f, 1.f, 0.0f };
    _triangle_mesh._vertices[1].color = { 0.f, 1.f, 0.0f };
    _triangle_mesh._vertices[2].color = { 0.f, 1.f, 0.0f };

    _monkey_mesh.loadFromObj("./assets/monkey_smooth.obj");

    uploadMesh(_triangle_mesh);
    uploadMesh(_monkey_mesh);

    _meshes["monkey"] = _monkey_mesh;
    _meshes["triangle"] = _triangle_mesh;
    // <++>
}

void VulkanEngine::uploadMesh(Mesh& mesh) {
    VkBufferCreateInfo buffer_info {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = mesh._vertices.size() * sizeof(Vertex);
    buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    VmaAllocationCreateInfo vma_alloc_info {};
    vma_alloc_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    errorCheck(vmaCreateBuffer(_allocator, 
                &buffer_info, 
                &vma_alloc_info, 
                &mesh._vertex_buffer._buffer, 
                &mesh._vertex_buffer._allocation, 
                nullptr));

    _main_deletion_queue.push_function(
            [=]() {
                vmaDestroyBuffer(_allocator, mesh._vertex_buffer._buffer, mesh._vertex_buffer._allocation);
            }
    );

    void* data;
    vmaMapMemory(_allocator, mesh._vertex_buffer._allocation, &data);
    memcpy(data, mesh._vertices.data(), mesh._vertices.size() * sizeof(Vertex));
    vmaUnmapMemory(_allocator, mesh._vertex_buffer._allocation);
}

VkPipeline PipelineBuilder::buildPipeline(VkDevice device, VkRenderPass pass) {
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

void VulkanEngine::drawObjects(VkCommandBuffer cmd, RenderObject* objects, int count) {
    glm::vec3 cam_pos = { 0.f, -3.f, -10.f };
    glm::mat4 view = glm::translate(glm::mat4(1.f), cam_pos);
    glm::mat4 projection = glm::perspective(glm::radians(70.f), 1700.f / 900.f, 0.1f, 200.f);
    projection[1][1] *= -1;

    Mesh* last_mesh = nullptr;
    Material* last_material = nullptr;

    for (int x = 0; x < count; x++) {
        RenderObject& object = objects[x];
        if (object.material != last_material) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
            last_material = object.material;
        }

        glm::mat4 model = object.transform_matrix;
        glm::mat4 mesh_matrix = projection * view * model;

        MeshPushConstants constants;
        constants.render_matrix = mesh_matrix;
        vkCmdPushConstants(cmd, object.material->pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);
        if (object.mesh != last_mesh) {
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &object.mesh->_vertex_buffer._buffer, &offset);
            last_mesh = object.mesh;
        }

        vkCmdDraw(cmd, object.mesh->_vertices.size(), 1, 0, 0);
    }
}

Material* VulkanEngine::createMaterial(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name) {
    Material mat;
    mat.pipeline = pipeline;
    mat.pipeline_layout = layout;
    _materials[name] = mat;
    return &_materials[name];
}

Material* VulkanEngine::getMaterial(const std::string& name) {
    auto it = _materials.find(name);
    if (it == _materials.end()) {
        return nullptr;
    }

    return &(*it).second;
}

Mesh* VulkanEngine::getMesh(const std::string& name) {
    auto it = _meshes.find(name);
    if (it == _meshes.end()) {
        return nullptr;
    }

    return &(*it).second;
}

FrameData& VulkanEngine::getCurrentFrame() {
    return _frames[_frame_number % FRAME_OVERLAP];
}
