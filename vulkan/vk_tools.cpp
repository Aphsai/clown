#include "vk_tools.hpp"
#include "shared.hpp"
#include "renderer.hpp"

void Tools::createBuffer(VkDeviceSize size, 
        VkBufferUsageFlags usage, 
        VkMemoryPropertyFlags properties,
        VkBuffer &buffer,
        VmaAllocation &allocation,
        VmaAllocator &allocator) {

    VkBufferCreateInfo buffer_info {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocate_info {};
    allocate_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocate_info.requiredFlags = properties;

    errorCheck(vmaCreateBuffer(allocator, &buffer_info, &allocate_info, &buffer, &allocation, nullptr));
}

VkCommandBuffer Tools::beginSingleCommand(Renderer* renderer) {
    VkCommandBufferAllocateInfo allocate_info {};

    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandPool = renderer->command_pool;
    allocate_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(renderer->device, &allocate_info, &command_buffer);
    
    VkCommandBufferBeginInfo begin_info {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &begin_info);
    return command_buffer;
}

void Tools::endSingleCommand(Renderer* renderer, VkCommandBuffer command_buffer) {
    vkEndCommandBuffer(command_buffer);
    VkSubmitInfo submit_info {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(renderer->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(renderer->graphics_queue);
    vkFreeCommandBuffers(renderer->device, renderer->command_pool, 1, &command_buffer);
}

void Tools::copyBuffer(Renderer* renderer, VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size) {
    VkCommandBuffer command_buffer = beginSingleCommand(renderer);
    VkBufferCopy copy_region {};
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = size;
    vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

    endSingleCommand(renderer, command_buffer);
}
