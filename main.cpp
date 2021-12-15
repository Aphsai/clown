#include "vulkan/vk_engine.hpp"
#include "ecs/coordinator.hpp"
#include <chrono>

struct Gravity {
    glm::vec3 position;
};

struct PhysicsSystem : System {
    void update(float dt) {

    }
};

int main() {
    VulkanEngine engine;

    engine.init();
    
    std::cout << "Clown is running!" << std::endl;
    Coordinator coordinator;

    coordinator.init(); // Initializes entity manager, system manager and component manager
    coordinator.register_component<Gravity>();

    auto physics_system = coordinator.register_system<PhysicsSystem>();

    Signature signature;
    signature.set(coordinator.get_component_type<Gravity>());

    coordinator.set_system_signature<PhysicsSystem>(signature); // Identify which components are going to be used in the system

    std::vector<Entity> entities (MAX_ENTITIES);

    for (auto& entity : entities) {
        entity = coordinator.create_entity();
        coordinator.add_component(entity, Gravity { glm::vec3(0.0f, -9.81f, 0.0f) });
    }

    float dt = 0.0f;
    
    bool quit = false;
    while(!quit) {
        auto start_time = std::chrono::high_resolution_clock::now();
        physics_system->update(dt);

        engine.update();
        auto stop_time = std::chrono::high_resolution_clock::now();
        dt = std::chrono::duration<float, std::chrono::seconds::period>(stop_time - start_time).count();
    }

    engine.cleanup();
}
