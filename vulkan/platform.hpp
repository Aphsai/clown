#define USE_FRAMEWORK_GLFW 1
#define GLFW_INCLUDE_VULKAN

#include <vector>
#include <stdint.h>
#include <GLFW/glfw3.h>

void initPlatform();
void destroyPlatform();
void addRequiredPlatformInstanceExtensions(std::vector<const char*> *instance_extensions);

