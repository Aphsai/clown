#include "window.hpp"
#include "vulkan_engine.hpp"
 
#include <assert.h>
#include <array>
#include <cstring>

Window::Window(VulkanEngine* engine, uint32_t size_x, uint32_t size_y, std::string name) {
    window_name = name;
    initOSWindow(engine, size_x, size_y);
    initOSSurface(size_x, size_y);
}

Window::~Window() {
    destroyOSWindow();
}


void Window::close() {
    window_should_run = false;
}

bool Window::update() {
    updateOSWindow();
    return window_should_run;
}

void Window::initOSWindow(VulkanEngine* engine, uint32_t size_x, uint32_t size_y) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfw_window = glfwCreateWindow(size_x, size_y, window_name.c_str(), nullptr, nullptr);
    if (!glfw_window) {
        glfwTerminate();
        assert(0 && "GLFW could not create window.");
        return;
    }
    glfwGetFramebufferSize(glfw_window, (int*)&(engine->surface_size_x), (int*)&(engine->surface_size_y));
}

void Window::updateOSWindow() {
    glfwPollEvents();
    if (glfwWindowShouldClose(glfw_window)) close();
}

void Window::initOSSurface(VulkanEngine *engine) {
    if (VK_SUCCESS != glfwCreateWindowSurface(engine->instance, glfw_window, nullptr, &(engine->surface))) {
        glfwTerminate();
        assert(0 && "GLFW could not create window surface.");
        return;
    }
}

void Window::destroyOSWindow() {
    glfwDestroyWindow(glfw_window);
}
