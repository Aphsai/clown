#pragma once 
#include "vk_types.hpp"

namespace vk_init {

    VkCommandPoolCreateInfo commandPoolCreateInfo(uint32_t queue_family_index, VkCommandPoolCreateFlags flags = 0);
    VkCommandBufferAllocateInfo commandBufferAllocateInfo(VkCommandPool pool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shader_module);
    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo();
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo(VkPrimitiveTopology topology);
    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo(VkPolygonMode polygon_mode);
    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo();
    VkPipelineColorBlendAttachmentState colorBlendAttachmentState();
    VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo(bool depth_test, bool depth_write, VkCompareOp compare_op);
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo();
    
    VkImageCreateInfo imageCreateInfo(VkFormat format, VkImageUsageFlags usage_flags, VkExtent3D extent);
    VkImageViewCreateInfo imageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspect_flags);


}
