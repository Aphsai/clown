#pragma once
#include "platform.hpp"
#include <iostream>
#include <assert.h>
#include <fstream>
#include <iostream>

void errorCheck(VkResult result);
uint32_t findMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties* gpu_memory_properties, const VkMemoryRequirements* memory_requirements, const VkMemoryPropertyFlags memory_properties);
uint32_t findMemoryType(VkPhysicalDevice gpu, uint32_t type_filter, VkMemoryPropertyFlags properties);
std::vector<char> readFile(const std::string& filename);
