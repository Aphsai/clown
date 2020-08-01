#pragma once
#include "platform.hpp"
#include <vector>
#include <string>

class Renderer;

struct Window {
        Window(Renderer *renderer, uint32_t size_x, uint32_t size_y, std::string name);
        ~Window();

        void close();
        bool update();
        void beginRender();
        void endRender(std::vector<VkSemaphore> wait_semaphores);

        void initOSWindow();
        void destroyOSWindow();
        void updateOSWindow();
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

        void initVertexBuffer();
        void destroyVertexBuffer();

        void initCommandBuffers();
        void destroyCommandBuffers();

        VkShaderModule initShaderModule(const std::vector<char>& code);

        Renderer *renderer = nullptr;

        std::string window_name;
        uint32_t surface_size_x = 512;
        uint32_t surface_size_y = 512;
        uint32_t swapchain_image_count = 2;
        uint32_t active_swapchain_image_id = UINT32_MAX;

        std::vector<VkImage> swapchain_images;
        std::vector<VkImageView> swapchain_image_views;
        std::vector<VkFramebuffer> framebuffers;
        std::vector<VkCommandBuffer> command_buffers;

        VkSurfaceFormatKHR surface_format = {};
        VkSurfaceCapabilitiesKHR surface_capabilities = {};
        VkExtent2D swapchain_extent = {};

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

        bool stencil_available = false;
        bool window_should_run = true;

        GLFWwindow *glfw_window = nullptr;

};
