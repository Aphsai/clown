#include "vk_engine.hpp"
#include "vk_types.hpp"
#include "vk_init.hpp"
#include "platform.hpp"

void VulkanEngine::init() {
    initPlatform();
    window = new Window(this, _window_extent.width, _window_extent.height, "Jester");
    _is_initialized = true;
}

void VulkanEngine::initVulkan() {
    vkb::InstanceBuilder builder;
    auto inst_ret = builder.set_app_name("Vulkan")
        .requestValidationLayers(true)
        .require_api_version(1,1,0)
        .use_default_debug_messenger()
        .build();
    vkb::Instance vkb_inst = inst_ret.value();

    _instance = vkb_inst.instance;
    _debug_messenger = vkb_inst.debug_messenger;
}

void VulkanEngine::cleanup() {
    if (_is_initialized) {
        delete window;
        destroyPlatform();
    }
}

void VulkanEngine::draw() {

}

void VulkanEngine::run() {
    bool quit = false;

    while(!quit) {
        window->update();
        quit = window->window_should_run;
        draw();
    }

}
