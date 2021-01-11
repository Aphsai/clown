#pragma once
#include "platform.hpp"
#include <vector>
#include <string>

class VulkanEngine;

struct Window {
        Window(VulkanEngine*, std::string name);
        ~Window();

        void close();
        bool update();

        void init_os_window(VulkanEngine*);
        void destroy_os_window();
        void update_os_window();
        void init_os_surface(VulkanEngine*);

        std::string window_name;
        bool window_should_run = true;


        GLFWwindow* glfw_window = nullptr;
};
