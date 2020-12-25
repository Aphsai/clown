#pragma once

#include "vk_mem_alloc.h"
#include "vk_bootstrap.h"

#include "vk_types.hpp"
#include "vk_mesh.hpp"
#include "window.hpp"
#include "shared.hpp"

#include <deque>
#include <functional>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

constexpr unsigned int FRAME_OVERLAP = 2; // frames to overlap when rendering

struct GPUSceneData {
    glm::vec4 fog_color;
    glm::vec4 fog_distances;
    glm::vec4 ambient_color;
    glm::vec4 sunlight_direction;
    glm::vec4 sunlight_color;
};

struct GPUCameraData {
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 viewproj;
};

struct FrameData {
    VkSemaphore _present_semaphore, _render_semaphore;
    VkFence _render_fence;

    VkCommandPool _command_pool;
    VkCommandBuffer _main_command_buffer;

    AllocatedBuffer camera_buffer;
    VkDescriptorSet global_descriptor;
};

struct Material {
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
};

struct RenderObject {
    Mesh* mesh;
    Material* material;

    glm::mat4 transform_matrix;
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
    VkPipelineDepthStencilStateCreateInfo _depth_stencil;
    VkPipelineLayout _pipeline_layout;

    VkPipeline buildPipeline(VkDevice device, VkRenderPass pass);
};

struct DeletionQueue {
    std::deque<std::function<void()>> deletors;
    
    void push_function(std::function<void()>&& function) {
        deletors.push_back(function);
    }

    void flush() {
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
            (*it)();
        }

        deletors.clear();
    }
};

struct MeshPushConstants {
    glm::vec4 data;
    glm::mat4 render_matrix;
};

struct VulkanEngine {

    bool _is_initialized = false;
    int _frame_number = 0;
    VkExtent2D _window_extent = { 1700, 900 };


    FrameData _frames[FRAME_OVERLAP];
    FrameData& getCurrentFrame();

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
    VkRenderPass _render_pass;
    std::vector<VkFramebuffer> _framebuffers;
    DeletionQueue _main_deletion_queue;
    VmaAllocator _allocator;
    VkImageView _depth_image_view;
    VkFormat _depth_format;
    VkDescriptorSetLayout _global_set_layout;
    VkDescriptorPool _descriptor_pool;
    VkPhysicalDeviceProperties _gpu_properties;
    GPUSceneData _scene_parameters;
    AllocatedBuffer _scene_parameter_buffer;

    std::vector<RenderObject> _renderables;
    std::unordered_map<std::string, Material> _materials;
    std::unordered_map<std::string, Mesh> _meshes;

    // <++>  tmp
    AllocatedImage _depth_image;
    Mesh _triangle_mesh;
    Mesh _monkey_mesh;
    VkPipelineLayout _triangle_pipeline_layout;
    VkPipeline _triangle_pipeline;
    // <++>

    Window* window; 
    
    void initVulkan();
    void initSwapchain();
    void initCommands();
    void initRenderPass();
    void initFramebuffers();
    void initSyncStructures();
    void initPipelines();
    void initScene();
    void initDescriptors();

    bool loadShaderModule(const char* file_path, VkShaderModule* out_shader_module);
    void loadMeshes();
    void uploadMesh(Mesh& mesh);
    void drawObjects(VkCommandBuffer cmd, RenderObject* first, int count);
    size_t padUniformBufferSize(size_t original_size);
    
    AllocatedBuffer createBuffer(size_t alloc_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage);
    Material* createMaterial(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name);
    Material* getMaterial(const std::string& name);
    Mesh* getMesh(const std::string& name);

    void init();
    void cleanup();
    void draw();
    void run();

};
