PROG = clown

SRCS := $(shell find . -name "*.cpp")
OBJS := $(SRCS:%=%.o)
DEPS := $(OBJS:.o=.d)
SHADERS := $(wildcard shaders/*.vert shaders/*.frag)
SPIRVS := $(addsuffix .spv, $(SHADERS))

CFLAGS = -std=c++17 -I$(VULKAN_SDK)/include
LDFLAGS = -g -L$(VULKAN_SDK)/lib `pkg-config --static --libs glfw3` -lvulkan
CC := g++

$(PROG): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.cpp.o: %.cpp
	$(CC) -MMD -MP $(CFLAGS) -c $< -o $@ $(LDFLAGS)

$(SPIRVS): %.spv: %
	glslc $< -o $@

.PHONY: clean

clean:

	find . -type f -name '*.o' -delete
	find . -type f -name '*.d' -delete
	find . -type f -name 'vgcore.*' -delete

.PHONY: shaders 

shaders: $(SPIRVS)

-include $(DEPS)
