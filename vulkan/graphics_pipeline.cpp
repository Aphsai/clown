#include "graphics_pipeline.hpp"
#include "renderer.hpp"
#include "mesh.hpp"
#include "shared.hpp"

GraphicsPipeline::GraphicsPipeline(Renderer* renderer) {
    this->renderer = renderer;
}

void GraphicsPipeline::createGraphicsPipeline(VkDescriptorSetLayout descriptor_set_layout) {
    VkPipelineLayoutCreateInfo pipeline_layout_info {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &descriptor_set_layout;
    
    errorCheck(vkCreatePipelineLayout(renderer->device, &pipeline_layout_info, nullptr, &pipeline_layout));

    auto vertex_shader_code = readFile("shaders/basic.vert.spv");
    auto frag_shader_code = readFile("shaders/basic.frag.spv");

    VkShaderModule vertex_shader_module = createShaderModule(vertex_shader_code);
    VkShaderModule frag_shader_module = createShaderModule(frag_shader_code);

    VkPipelineShaderStageCreateInfo vert_shader_stage_create_info {};
    vert_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_create_info.module = vertex_shader_module;
    vert_shader_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_shader_stage_create_info {};
    frag_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_create_info.module = frag_shader_module;
    frag_shader_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages [] = { vert_shader_stage_create_info, frag_shader_stage_create_info };

    auto binding_description = Vertex::getBindingDescription();
    auto attribute_descriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertex_input_info {};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_info.vertexAttributeDescriptionCount = attribute_descriptions.size();
    vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info {};
    input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_create_info.primitiveRestartEnable = VK_FALSE;
   
    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info {};
    rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_create_info.depthClampEnable = VK_FALSE;
    rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state_create_info.lineWidth = 1.0f;
    rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterization_state_create_info.depthBiasEnable = VK_FALSE;
    rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
    rasterization_state_create_info.depthBiasClamp = 0.0f;
    rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo ms_state_info {};
    ms_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms_state_info.sampleShadingEnable = VK_FALSE;
    ms_state_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState color_blend_attachment {};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blend_state_info {};
    color_blend_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_info.attachmentCount = 1;
    color_blend_state_info.pAttachments = &color_blend_attachment;

    VkViewport viewport {};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = (float) renderer->swapchain_extent.width;
    viewport.height = (float) renderer->swapchain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor {};
    scissor.offset = { 0, 0 };
    scissor.extent = renderer->swapchain_extent;
    
    VkPipelineViewportStateCreateInfo viewport_state_info {};
    viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_info.viewportCount = 1;
    viewport_state_info.pViewports = &viewport;
    viewport_state_info.scissorCount = 1;
    viewport_state_info.pScissors = &scissor;

    VkGraphicsPipelineCreateInfo graphics_create_info {};
    graphics_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphics_create_info.stageCount = 2;
    graphics_create_info.pStages = shader_stages;
    graphics_create_info.pVertexInputState = &vertex_input_info;
    graphics_create_info.pInputAssemblyState = &input_assembly_create_info;
    graphics_create_info.pRasterizationState = &rasterization_state_create_info;
    graphics_create_info.pMultisampleState = &ms_state_info;
    graphics_create_info.pDepthStencilState = nullptr;
    graphics_create_info.pColorBlendState = &color_blend_state_info;
    graphics_create_info.pDynamicState = nullptr;
    graphics_create_info.pViewportState = &viewport_state_info;

    graphics_create_info.layout = pipeline_layout;
    graphics_create_info.renderPass = renderer->render_pass;
    graphics_create_info.subpass = 0;

    errorCheck(vkCreateGraphicsPipelines(renderer->device, VK_NULL_HANDLE, 1, &graphics_create_info, nullptr, &pipeline));

    vkDestroyShaderModule(renderer->device, vertex_shader_module, nullptr);
    vkDestroyShaderModule(renderer->device, frag_shader_module, nullptr);
}

void GraphicsPipeline::destroy() {
    vkDestroyPipeline(renderer->device, pipeline, nullptr);
    vkDestroyPipelineLayout(renderer->device, pipeline_layout, nullptr);
}

VkShaderModule GraphicsPipeline::createShaderModule(const std::vector<char> &code) {
    VkShaderModuleCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shader_module;

    errorCheck(vkCreateShaderModule(renderer->device, &create_info, nullptr, &shader_module));
    return shader_module;
}

