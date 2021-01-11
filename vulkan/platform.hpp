#define USE_FRAMEWORK_GLFW 1
#define GLFW_INCLUDE_VULKAN

#include <vector>
#include <stdint.h>
#include <GLFW/glfw3.h>

void init_platform();
void destroy_platform();
void add_required_platform_instance_extensions(std::vector<const char*> *instance_extensions);

