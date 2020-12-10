#pragma once

#include "vk_types.hpp"
#include "window.hpp"

struct VulkanEngine {
    bool _is_initialized = false;
    int _frame_number = 0;
    VkExtent2D _window_extent = { 1700, 900 };

    Window* window; 

    void init();
    void cleanup();
    void draw();
    void run();

};

