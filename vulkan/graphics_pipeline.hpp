#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <fstream>

class Renderer;

class GraphicsPipeline {
    public:
        GraphicsPipeline(Renderer*);

        Renderer* renderer;

        VkPipelineLayout pipeline_layout;
        VkPipeline pipeline;

        void createGraphicsPipeline(VkDescriptorSetLayout descriptor_set_layout);
        VkShaderModule createShaderModule(const std::vector<char> &code);
        void destroy();

};
