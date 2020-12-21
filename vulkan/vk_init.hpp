#pragma once 
#include "vk_types.hpp"

namespace vk_init {

    VkCommandPoolCreateInfo commandPoolCreateInfo(uint32_t queue_family_index, VkCommandPoolCreateFlags flags = 0);
    VkCommandBufferAllocateInfo commandBufferAllocateInfo(VkCommandPool pool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shader_module);

}
