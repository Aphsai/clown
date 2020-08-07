#pragma once 
#include "platform.hpp"
#include "vk_mem_alloc.h"
#include <vector>
#include <string>

class Window;

class Renderer {
    public:
        Renderer (uint32_t size_x, uint32_t size_y, std::string name);
        ~Renderer();
        bool run();
        void setupLayersAndExtensions();

        void beginRender();
        void endRender(std::vector<VkSemaphore> wait_semaphores);
        void initInstance();
        void destroyInstance();
        void initDevice();
        void destroyDevice();
        void setupDebug();
        void initDebug();
        void destroyDebug();
        void initOSSurface();
        void initSurface();
        void destroySurface();
        void initSwapchain();
        void destroySwapchain();
        void initSwapchainImages();
        void recreateSwapchain();
        void destroySwapchainImages();
        void initDepthStencilImage();
        void destroyDepthStencilImage();
        void initRenderPass();
        void destroyRenderPass();
        void initFramebuffers();
        void destroyFramebuffers();
        void initSynchronizations();
        void destroySynchronizations();
        void initGraphicsPipeline();
        void destroyGraphicsPipeline();
        void initCommandPool();
        void destroyCommandPool();
        void initTextureImage();
        void destroyTextureImage();
        void initTextureImageView();
        void destroyTextureImageView();
        void initTextureSampler();
        void destroyTextureSampler();
        void initVertexBuffer();
        void destroyVertexBuffer();
        void initCommandBuffers();
        void destroyCommandBuffers();
        void initVMAAllocator();
        void destroyVMAAllocator();
        void initDescriptorSetLayout();
        void initDescriptorPool();
        void initDescriptorSets();

        void initIndexBuffer();
        void initUniformBuffer();

        
        // Helpers
        VkShaderModule createShaderModule(const std::vector<char>& code);
        VmaAllocation createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer);
        VkCommandBuffer beginSingleCommand();
        void endSingleCommand(VkCommandBuffer command_buffer);
        void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &image_memory);
        void transitionLayout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout);
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);


        VkPipeline graphics_pipeline = VK_NULL_HANDLE;
        VkFence swapchain_image_available = VK_NULL_HANDLE;
        VkImage depth_stencil_image = VK_NULL_HANDLE;
        VkDeviceMemory depth_stencil_image_memory = VK_NULL_HANDLE;
        VkImageView depth_stencil_image_view = VK_NULL_HANDLE;
        VkFormat depth_stencil_format = VK_FORMAT_UNDEFINED;
        VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        VkRenderPass render_pass = VK_NULL_HANDLE;
        VkCommandPool command_pool = VK_NULL_HANDLE;
        VkBuffer vertex_buffer = VK_NULL_HANDLE;
        VkDeviceMemory vertex_buffer_memory = VK_NULL_HANDLE;
        VkInstance instance = VK_NULL_HANDLE;
        VkPhysicalDevice gpu = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        VkQueue queue = VK_NULL_HANDLE;
        VkQueue present_queue = VK_NULL_HANDLE;
        VkImage texture_image = VK_NULL_HANDLE;
        VkDeviceMemory texture_image_memory = VK_NULL_HANDLE;
        VkImageView texture_image_view = VK_NULL_HANDLE;
        VkSampler texture_sampler = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
        VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
        VkBuffer index_buffer = VK_NULL_HANDLE;

        VkPhysicalDeviceProperties gpu_properties = {};
        VkPhysicalDeviceMemoryProperties gpu_memory_properties = {};
        VkSurfaceFormatKHR surface_format = {};
        VkSurfaceCapabilitiesKHR surface_capabilities = {};
        VkExtent2D swapchain_extent = {};

        uint32_t graphics_family_index = -1;
        uint32_t present_family_index = -1;
        uint32_t swapchain_image_count = 2;
        uint32_t active_swapchain_image_id = UINT32_MAX;
        uint32_t surface_size_x = 512;
        uint32_t surface_size_y = 512;
        bool stencil_available = false;

        Window* window = nullptr;

        VmaAllocator allocator;
        VmaAllocation allocation;

        std::vector<const char*> instance_layers;
        std::vector<const char*> instance_extensions;
        std::vector<const char*> device_extensions;

        std::vector<VkImage> swapchain_images;
        std::vector<VkImageView> swapchain_image_views;
        std::vector<VkFramebuffer> framebuffers;
        std::vector<VkCommandBuffer> command_buffers;
        std::vector<VkDescriptorSet> descriptor_sets;
        std::vector<VkBuffer> uniform_buffers;
        std::vector<VmaAllocation> uniform_buffer_allocators;

        // DEBUG
        VkDebugReportCallbackEXT debug_report = VK_NULL_HANDLE;
        VkDebugReportCallbackCreateInfoEXT debug_callback_create_info = {};
};
