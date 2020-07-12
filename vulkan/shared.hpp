#pragma once
#include "platform.hpp"
#include <iostream>
#include <assert.h>

void errorCheck(VkResult result);
uint32_t findMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties* gpu_memory_properties, const VkMemoryRequirements* memory_requirements, const VkMemoryPropertyFlags memory_properties);
