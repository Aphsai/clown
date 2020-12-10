#include <array>
#include <chrono>
#include <iostream>
#include <cmath>
#include <glm/glm.hpp>

#include "vk_engine.hpp"

int main(int argc, char* argv []) {
    
    VulkanEngine engine;
    engine.init();
    engine.run();
    engine.cleanup();

	return 0;
}
