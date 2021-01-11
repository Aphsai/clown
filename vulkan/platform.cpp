#include "platform.hpp"
#include <assert.h>

void init_platform() {
    glfwInit();
}

void destroy_platform() {
    glfwTerminate();
}

void add_required_platform_instance_extensions(std::vector<const char*> *instance_extensions) {
    uint32_t instance_extension_count = 0;
    const char** instance_extensions_buffer = glfwGetRequiredInstanceExtensions(&instance_extension_count);
    for (uint32_t x = 0; x < instance_extension_count; x++) {
        instance_extensions->push_back(instance_extensions_buffer[x]);
    }
}
