#pragma once
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

class Renderer;

namespace Tools {
    void createBuffer(VkDeviceSize size, 
            VkBufferUsageFlags usage, 
            VkMemoryPropertyFlags properties,
            VkBuffer &buffer,
            VmaAllocation &allocation,
            VmaAllocator &allocator);

    VkCommandBuffer beginSingleCommand(Renderer* renderer);
    void endSingleCommand(Renderer* renderer, VkCommandBuffer command_buffer);
    void copyBuffer(Renderer* renderer, VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);
}
