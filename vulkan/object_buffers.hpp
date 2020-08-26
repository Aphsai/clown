#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "vk_mem_alloc.h"
#include "mesh.hpp"

class Renderer;

class ObjectBuffers {
    public:
        ObjectBuffers(Renderer*);

        std::vector<Vertex> vertices;
        VkBuffer vertex_buffer;
        VmaAllocation vertex_allocation;

        std::vector<uint32_t> indices;
        VkBuffer index_buffer;
        VmaAllocation index_allocation;

        VkBuffer uniform_buffers;
        VmaAllocation uniform_allocation;

        Renderer* renderer;

        void createVertexBuffer();
        void createIndexBuffer();
        void createUniformBuffers();
        void destroy();
};

