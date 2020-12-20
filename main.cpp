#include "vulkan/vk_engine.hpp"

int main() {
    VulkanEngine engine;

    engine.init();
    engine.run();
    engine.cleanup();

}
