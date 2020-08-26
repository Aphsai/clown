#include <array>
#include <chrono>
#include <iostream>
#include <cmath>
#include <glm/glm.hpp>

#include "renderer.hpp"
#include "window.hpp"
#include "shared.hpp"
#include "camera.hpp"
#include "object_renderer.hpp"



int main() {
	Renderer* r = new Renderer(800, 600, "CLOWN");
	auto w = r->window;
    
    Camera camera;
    camera.init(45.0f, 128.0f, 720.0f, 0.1f, 10000.0f);
    camera.pos = glm::vec3(0.0f, 0.0f, 4.0f);

    ObjectRenderer object(r);
    object.createObjectRenderer(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.5f));

	while(r->run()) {
        r->drawBegin();

            object.updateUniformBuffer(camera);
            object.draw();

        r->drawEnd();

        w->update();
	}
    object.destroy(); 
    delete r;
	return 0;
}
