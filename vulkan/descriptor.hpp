#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class Renderer;

class Descriptor {
    public:
        Descriptor(Renderer*);

        VkDescriptorSetLayout set_layout;
        VkDescriptorPool pool;
        VkDescriptorSet set;

        Renderer *renderer;

        void createLayoutSetPoolAndAllocate(uint32_t swapchain_image_count);
        void createPoolAndAllocateSets(uint32_t swapchain_image_count);
        void populateSets(uint32_t swapchain_image_count, VkBuffer uniform_buffers);
        void createSetLayout();
        void destroy();
};
