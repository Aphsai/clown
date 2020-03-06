CFLAGS = -std=c++17 -I$(VULKAN_SDK)/include

LDFLAGS = -L$(VULKAN_SDK)/lib `pkg-config --static --libs glfw3` -lvulkan

VulkanTest: glfw-test.cpp
	g++ $(CFLAGS) -o VulkanTest glfw-test.cpp $(LDFLAGS)

.PHONY: test clean

test: VulkanTest
	./VulkanTest

clean:
	rm -f VulkanTest
