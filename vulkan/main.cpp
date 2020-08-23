#include <array>
#include <chrono>
#include <iostream>
#include <cmath>
#include <glm/glm.hpp>

#include "renderer.hpp"
#include "window.hpp"
#include "shared.hpp"



int main() {
	Renderer* r = new Renderer(800, 600, "CLOWN");
	auto w = r->window;
    

	while(r->run()) {
        r->drawBegin();

        r->drawEnd();

        w->update();
	}
    
    delete r;
	return 0;
}
