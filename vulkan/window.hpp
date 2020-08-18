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

        void initOSWindow(Renderer*);
        void destroyOSWindow();
        void updateOSWindow();
        void initOSSurface(Renderer*);

        std::string window_name;
        bool window_should_run = true;


        GLFWwindow* glfw_window = nullptr;
};
