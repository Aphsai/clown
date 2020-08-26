#include "descriptor.hpp"
#include "renderer.hpp"
#include "shared.hpp"
#include "mesh.hpp"
#include <array>

Descriptor::Descriptor(Renderer* renderer) {
    this->renderer = renderer;
}

void Descriptor::createLayoutSetPoolAndAllocate(uint32_t swapchain_image_count) {
   createSetLayout();
   createPoolAndAllocateSets(swapchain_image_count);
}

void Descriptor::createSetLayout() {
    VkDescriptorSetLayoutBinding ubo_layout_binding {};
    ubo_layout_binding.binding = 0;
    ubo_layout_binding.descriptorCount = 1;
    ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::array<VkDescriptorSetLayoutBinding, 1> layout_bindings = { ubo_layout_binding };

    VkDescriptorSetLayoutCreateInfo layout_create_info {};
    layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_create_info.bindingCount = layout_bindings.size();
    layout_create_info.pBindings = layout_bindings.data();

    errorCheck(vkCreateDescriptorSetLayout(renderer->device, &layout_create_info, nullptr, &set_layout));
}

void Descriptor::createPoolAndAllocateSets(uint32_t swapchain_image_count) {
    std::array<VkDescriptorPoolSize, 1> pool_sizes {};

    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = swapchain_image_count;

    VkDescriptorPoolCreateInfo pool_info {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = pool_sizes.size();
    pool_info.pPoolSizes = pool_sizes.data();
    pool_info.maxSets = swapchain_image_count;

    errorCheck(vkCreateDescriptorPool(renderer->device, &pool_info, nullptr, &pool));

    std::vector<VkDescriptorSetLayout> layouts(swapchain_image_count, set_layout);
    VkDescriptorSetAllocateInfo allocate_info {};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = pool;
    allocate_info.descriptorSetCount = swapchain_image_count;
    allocate_info.pSetLayouts = layouts.data();

    errorCheck(vkAllocateDescriptorSets(renderer->device, &allocate_info, &set));
}

void Descriptor::populateSets(uint32_t swapchain_image_count, VkBuffer uniform_buffers) {
    for (size_t x = 0; x < swapchain_image_count; x++) {
        VkDescriptorBufferInfo ubo_buffer_descriptor_info {};
        ubo_buffer_descriptor_info.buffer = uniform_buffers;
        ubo_buffer_descriptor_info.offset = 0;
        ubo_buffer_descriptor_info.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet ubo_descriptor_write {};
        ubo_descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        ubo_descriptor_write.pNext = nullptr;
        ubo_descriptor_write.dstSet = set;
        ubo_descriptor_write.dstBinding = 0;
        ubo_descriptor_write.dstArrayElement = 0;
        ubo_descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ubo_descriptor_write.descriptorCount = 1;
        ubo_descriptor_write.pBufferInfo = &ubo_buffer_descriptor_info;
        ubo_descriptor_write.pImageInfo = nullptr;
        ubo_descriptor_write.pTexelBufferView = nullptr;

        std::array<VkWriteDescriptorSet, 1> descriptor_writes = { ubo_descriptor_write };

        vkUpdateDescriptorSets(renderer->device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
    }
}

void Descriptor::destroy() {
    vkDestroyDescriptorPool(renderer->device, pool, nullptr);
    vkDestroyDescriptorSetLayout(renderer->device, set_layout, nullptr);
}
