#include <cstring>
#include <iostream>
#include "object_buffers.hpp"
#include "vk_tools.hpp"
#include "renderer.hpp"

ObjectBuffers::ObjectBuffers(Renderer* render) {
    renderer = render;
}

void ObjectBuffers::createVertexBuffer() {
    VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();
    VkBuffer staging_buffer;
    VmaAllocation staging_allocation;
    
    Tools::createBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_allocation, renderer->allocator);
    void* data;
    std::cout << staging_buffer << " " << staging_allocation  << std::endl;
    vmaMapMemory(renderer->allocator, staging_allocation, &data);
    memcpy(data, vertices.data(), buffer_size);
    vmaUnmapMemory(renderer->allocator, staging_allocation);

    Tools::createBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_buffer, vertex_allocation, renderer->allocator);
    Tools::copyBuffer(renderer, staging_buffer, vertex_buffer, buffer_size);
    vmaDestroyBuffer(renderer->allocator, staging_buffer, staging_allocation);
}

void ObjectBuffers::createIndexBuffer() {
    VkDeviceSize buffer_size = sizeof(indices[0]) * indices.size();
    VkBuffer staging_buffer;
    VmaAllocation staging_allocation;
    
    Tools::createBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_allocation, renderer->allocator);
    void* data;
    vmaMapMemory(renderer->allocator, staging_allocation, &data);
    memcpy(data, indices.data(), buffer_size);
    vmaUnmapMemory(renderer->allocator, staging_allocation);

    Tools::createBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, index_buffer, index_allocation, renderer->allocator);
    Tools::copyBuffer(renderer, staging_buffer, index_buffer, buffer_size);
    vmaDestroyBuffer(renderer->allocator, staging_buffer, staging_allocation);
}


void ObjectBuffers::createUniformBuffers() {
    VkDeviceSize buffer_size = sizeof(UniformBufferObject);
    Tools::createBuffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniform_buffers, uniform_allocation, renderer->allocator);
}

void ObjectBuffers::destroy() {
    vmaDestroyBuffer(renderer->allocator, vertex_buffer, vertex_allocation);
    vmaDestroyBuffer(renderer->allocator, index_buffer, index_allocation);
    vmaDestroyBuffer(renderer->allocator, uniform_buffers, uniform_allocation);
}
