#include "window.hpp"
#include "renderer.hpp"
 
#include <assert.h>
#include <array>
#include <cstring>

Window::Window(Renderer* renderer, uint32_t size_x, uint32_t size_y, std::string name) {
    window_name = name;
    initOSWindow(renderer);
    initOSSurface(renderer);
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

void Window::initOSWindow(Renderer* renderer) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfw_window = glfwCreateWindow(renderer->surface_size_x, renderer->surface_size_y, window_name.c_str(), nullptr, nullptr);
    if (!glfw_window) {
        glfwTerminate();
        assert(0 && "GLFW could not create window.");
        return;
    }
    glfwGetFramebufferSize(glfw_window, (int*)&(renderer->surface_size_x), (int*)&(renderer->surface_size_y));
}

void Window::updateOSWindow() {
    glfwPollEvents();
    if (glfwWindowShouldClose(glfw_window)) close();
}

void Window::initOSSurface(Renderer* renderer) {
    if (VK_SUCCESS != glfwCreateWindowSurface(renderer->instance, glfw_window, nullptr, &(renderer->surface))) {
        glfwTerminate();
        assert(0 && "GLFW could not create window surface.");
        return;
    }
}

void Window::destroyOSWindow() {
    glfwDestroyWindow(glfw_window);
}
