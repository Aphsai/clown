#include "object_renderer.hpp"
#include "renderer.hpp"
#include "mesh.hpp"
#include <cstring>

ObjectRenderer::ObjectRenderer(Renderer* renderer) {
    this->renderer = renderer;

    graphics_pipeline = new GraphicsPipeline(renderer);
    object_buffers = new ObjectBuffers(renderer);
    descriptor = new Descriptor(renderer);
}

ObjectRenderer::~ObjectRenderer() {
    delete graphics_pipeline;
    delete object_buffers;
    delete descriptor;
}

void ObjectRenderer::createObjectRenderer(glm::vec3 position, glm::vec3 scale) {
    uint32_t swapchain_image_count = renderer->swapchain_image_count;
    VkExtent2D swapchain_extent = renderer->swapchain_extent;

    object_buffers->createVertexBuffer();
    object_buffers->createIndexBuffer();
    object_buffers->createUniformBuffers();

    descriptor->createLayoutSetPoolAndAllocate(swapchain_image_count);
    descriptor->populateSets(swapchain_image_count, object_buffers->uniform_buffers);

    graphics_pipeline->createGraphicsPipeline(descriptor->set_layout);

    this->position = position;
    this->scale = scale;
}

void ObjectRenderer::updateUniformBuffer(Camera camera) {
    UniformBufferObject ubo = {};

    glm::mat4 scale_matrix = glm::mat4(1.0f);
    glm::mat4 rot_matrix = glm::mat4(1.0f);
    glm::mat4 trans_matrix = glm::mat4(1.0f);

    scale_matrix = glm::scale(glm::mat4(1.0f), scale);
    trans_matrix = glm::translate(glm::mat4(1.0f), position);

    ubo.model = trans_matrix * rot_matrix * scale_matrix;
    ubo.view = camera.view_matrix;
    ubo.proj = camera.project_matrix;

    ubo.proj[1][1] *= -1;

    void* data;
    vmaMapMemory(renderer->allocator, object_buffers->uniform_allocation, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vmaUnmapMemory(renderer->allocator, object_buffers->uniform_allocation);

}
void ObjectRenderer::draw() {
    VkCommandBuffer command_buffer = renderer->command_buffers[renderer->active_swapchain_image_id];
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline->pipeline);
    VkBuffer vertex_buffers[] = { object_buffers->vertex_buffer };
    VkDeviceSize offsets[] = { 0 };

    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
    vkCmdBindIndexBuffer(command_buffer, object_buffers->index_buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline->pipeline_layout, 0, 1, &(descriptor->set), 0, nullptr);
    vkCmdDrawIndexed(command_buffer, object_buffers->indices.size(), 1, 0, 0, 0);
}

void ObjectRenderer::destroy() {
    graphics_pipeline->destroy();
    descriptor->destroy();
    object_buffers->destroy();
}
