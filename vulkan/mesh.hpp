#pragma once
#include <glm/glm.hpp>
#include <array>

#define GLM_FORCE_RADIAN
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 color;

    glm::vec2 tex_coords;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription binding_description {};
        binding_description.binding = 0;
        binding_description.stride = sizeof(Vertex);
        binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return binding_description;
    }

    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 4> attribute_descriptions {};
        attribute_descriptions[0].binding = 0;
        attribute_descriptions[0].location = 0;
        attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[0].offset = offsetof(Vertex, pos);

        attribute_descriptions[1].binding = 0;
        attribute_descriptions[1].location = 1;
        attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[1].offset = offsetof(Vertex, normal);

        attribute_descriptions[2].binding = 0;
        attribute_descriptions[2].location = 2;
        attribute_descriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[2].offset = offsetof(Vertex, color);
        
        attribute_descriptions[3].binding = 0;
        attribute_descriptions[3].location = 3;
        attribute_descriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
        attribute_descriptions[3].offset = offsetof(Vertex, tex_coords);

        return attribute_descriptions;
    }
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};


