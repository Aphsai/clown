#define TINYOBJLOADER_IMPLEMENTATION
#include "vk_mesh.hpp"
#include "tiny_obj_loader.h"
#include <iostream>

VertexInputDescription Vertex::get_vertex_description() {
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

    VkVertexInputAttributeDescription uv_attribute {};
    uv_attribute.binding = 0;
    uv_attribute.location = 3;
    uv_attribute.format = VK_FORMAT_R32G32_SFLOAT;
    uv_attribute.offset = offsetof(Vertex, uv);

    description.attributes.push_back(position_attribute);
    description.attributes.push_back(normal_attribute);
    description.attributes.push_back(color_attribute);
    description.attributes.push_back(uv_attribute);
    
    return description;
}

bool Mesh::load_from_obj(const char* file_name, const char* dir_name) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;

    tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file_name, dir_name);
    if (!warn.empty()) {
        std::cout << "Warn: " << warn << std::endl;
    }

    if (!err.empty()) {
        std::cerr << err << std::endl;
        return false;
    }

    for (size_t s = 0; s < shapes.size(); s++) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            int fv = 3; // hardcode to load triangles

            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
                tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
                tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];

                tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
                tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
                tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];

                tinyobj::real_t ux = attrib.texcoords[2 * idx.texcoord_index + 0];
                tinyobj::real_t uy = attrib.texcoords[2 * idx.texcoord_index + 1];

                Vertex new_vert;
                new_vert.position.x = vx;
                new_vert.position.y = vy;
                new_vert.position.z = vz;

                new_vert.normal.x = nx;
                new_vert.normal.y = ny;
                new_vert.normal.z = nz;

                new_vert.color = new_vert.normal;

                new_vert.uv.x = ux;
                new_vert.uv.y = 1 - uy;

                _vertices.push_back(new_vert);
            }

            index_offset += fv;
        }
    }
    
    return true;
}
