#include "window.hpp"
#include "vk_engine.hpp"
 
#include <assert.h>
#include <array>
#include <cstring>

Window::Window(VulkanEngine* engine, std::string name) {
    window_name = name;
    initOSWindow(engine);
    initOSSurface(engine);
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

void Window::initOSWindow(VulkanEngine* engine) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfw_window = glfwCreateWindow(engine->_window_extent.width, engine->_window_extent.height, window_name.c_str(), nullptr, nullptr);
    if (!glfw_window) {
        glfwTerminate();
        assert(0 && "GLFW could not create window.");
        return;
    }
    glfwGetFramebufferSize(glfw_window, (int*)&(engine->_window_extent.width), (int*)&(engine->_window_extent.height));
}

void Window::updateOSWindow() {
    glfwPollEvents();
    if (glfwWindowShouldClose(glfw_window)) close();
}

void Window::initOSSurface(VulkanEngine *engine) {
    if (VK_SUCCESS != glfwCreateWindowSurface(engine->_instance, glfw_window, nullptr, &(engine->_surface))) {
        glfwTerminate();
        assert(0 && "GLFW could not create window surface.");
        return;
    }
}

void Window::destroyOSWindow() {
    glfwDestroyWindow(glfw_window);
}
