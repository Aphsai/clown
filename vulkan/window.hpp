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

        void initOSWindow(VulkanEngine*);
        void destroyOSWindow();
        void updateOSWindow();
        void initOSSurface(VulkanEngine*);

        std::string window_name;
        bool window_should_run = true;


        GLFWwindow* glfw_window = nullptr;
};
