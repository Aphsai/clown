#pragma once

#include "vk_types.hpp"
#include "window.hpp"
#include "vk_bootstrap.h"
#include "shared.hpp"

struct VulkanEngine {

    bool _is_initialized = false;
    int _frame_number = 0;
    VkExtent2D _window_extent = { 1700, 900 };

    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debug_messenger;
    VkPhysicalDevice _gpu;
    VkDevice _device;
    VkSurfaceKHR _surface;
    VkSwapchainKHR _swapchain;
    VkFormat _swapchain_image_format;
    std::vector<VkImage> _swapchain_images;
    std::vector<VkImageView> _swapchain_image_views;
    VkQueue _graphics_queue;
    uint32_t _graphics_queue_family;
    VkCommandPool _command_pool;
    VkCommandBuffer _main_command_buffer;
    VkRenderPass _render_pass;
    std::vector<VkFramebuffer> _framebuffers;
    VkSemaphore _present_semaphore;
    VkSemaphore _render_semaphore;
    VkFence _render_fence;

    // ---  tmp
    VkPipelineLayout _triangle_pipeline_layout;
    VkPipeline _triangle_pipeline;
    // ----

    Window* window; 
    
    void initVulkan();
    void initSwapchain();
    void initCommands();
    void initRenderPass();
    void initFramebuffers();
    void initSyncStructures();
    void initPipelines();

    bool loadShaderModule(const char* file_path, VkShaderModule* out_shader_module);

    void init();
    void cleanup();
    void draw();
    void run();

};

struct PipelineBuilder {
    std::vector<VkPipelineShaderStageCreateInfo> _shader_stages;
    VkPipelineVertexInputStateCreateInfo _vertex_input_info;
    VkPipelineInputAssemblyStateCreateInfo _input_assembly;
    VkViewport _viewport;
    VkRect2D _scissor;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState _color_blend_attachment;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineLayout _pipeline_layout;

    VkPipeline buildPipeline(VkDevice device, VkRenderPass pass);
};

