#include "vk_init.hpp"

VkCommandPoolCreateInfo vk_init::commandPoolCreateInfo(uint32_t queue_family_index, VkCommandPoolCreateFlags flags) {
    VkCommandPoolCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.pNext = nullptr;
    info.queueFamilyIndex = queue_family_index;
    info.flags = flags;
    return info;
}

VkCommandBufferAllocateInfo vk_init::commandBufferAllocateInfo(VkCommandPool pool, uint32_t count, VkCommandBufferLevel level) {
    VkCommandBufferAllocateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.pNext = nullptr;
    info.commandPool = pool;
    info.commandBufferCount = count;
    info.level = level;
    return info;
}

VkPipelineShaderStageCreateInfo vk_init::pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shader_module) {
    VkPipelineShaderStageCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.pNext = nullptr;
    info.stage = stage;
    info.module = shader_module;
    info.pName = "main";
    return info;
}

VkPipelineVertexInputStateCreateInfo vk_init::vertex_input_state_create_info() {
    VkPipelineVertexInputStateCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    info.pNext = nullptr;
    info.vertexBindingDescriptionCount = 0;
    info.vertexAttributeDescriptionCount = 0;
    return info;
}
