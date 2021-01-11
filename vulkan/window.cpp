#include "window.hpp"
#include "vk_engine.hpp"
 
#include <assert.h>
#include <array>
#include <cstring>

Window::Window(VulkanEngine* engine, std::string name) {
    window_name = name;
    init_os_window(engine);
    init_os_surface(engine);
}

Window::~Window() {
    destroy_os_window();
}


void Window::close() {
    window_should_run = false;
}

bool Window::update() {
    update_os_window();
    return window_should_run;
}

void Window::init_os_window(VulkanEngine* engine) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfw_window = glfwCreateWindow(engine->_window_extent.width, engine->_window_extent.height, window_name.c_str(), nullptr, nullptr);
    if (!glfw_window) {
        glfwTerminate();
        assert(0 && "GLFW could not create window.");
        return;
    }
    glfwGetFramebufferSize(glfw_window, (int*)&(engine->_window_extent.width), (int*)&(engine->_window_extent.height));
}

void Window::update_os_window() {
    glfwPollEvents();
    if (glfwWindowShouldClose(glfw_window)) close();
}

void Window::init_os_surface(VulkanEngine *engine) {
    if (VK_SUCCESS != glfwCreateWindowSurface(engine->_instance, glfw_window, nullptr, &(engine->_surface))) {
        glfwTerminate();
        assert(0 && "GLFW could not create window surface.");
        return;
    }
}

void Window::destroy_os_window() {
    glfwDestroyWindow(glfw_window);
}
