#pragma once

#include "vk_types.hpp"

#include <vector>
#include <glm/vec3.hpp>

struct VertexInputDescription {
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexAttributeBindingDescription> attributes;

    VkPipelineVertexInputStateCreateFlags flags = 0;
};

VertexInputDescription Vertex::getVertexDescription() {
    VertexInputDescription description;

    VkVertexInputBindingDescription main_binding {};
    main_binding.binding = 0;
    main_binding.stride = sizeof(Vertex);
    main_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    description.bindings.push_back(main_binding);

    VkVertexInputAttributeDescription position_attribute {};
    position_attribute.binding = 0;
    position_attribute.location = 0;
    position_attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    position_attribute.offset = offsetof(Vertex, position);

    VkVertexInputAttributeDescription normal_attribute {};
    normal_attribute.binding = 0;
    normal_attribute.location = 1;
    normal_attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    normal_attribute.offset = offsetof(Vertex, normal);

    VkVertexInputAttributeDescription color_attribute {};
    color_attribute.binding = 0;
    color_attribute.location = 2;
    color_attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    color_attribute.offset = offsetof(Vertex, color);

    description.attributes.push_back(position_attribute);
    description.attributes.push_back(normal_attribute);
    description.attributes.push_back(color_attribute);
    
    return description;
}

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;

    static VertexInputDescription getVertexDescription();
};

struct Mesh {
    std::vector<Vertex> _vertices;
    AllocatedBuffer _vertex_buffer;
};
