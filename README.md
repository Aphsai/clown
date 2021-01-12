# Clown

## Entity Component System

In order to ease game development on a non graphical user interface, an entity-component-system architecture was chosen and implemented. It's managed through the coordinator class and an example of its usage and conciseness can be seen:

Example Components:
```cpp
struct Gravity {
    glm::vec3 force;
};

struct Velocity {
    glm::vec3 velocity;
};
```

Example System:
```cpp
extern Coordinator coord;

void PhysicsSystem::update(float dt) {
    for (auto const& entity : entities) {
        auto const& gravity = coord.get_component<Gravity>(entity);
        auto& velocity = coord.get_component<Velocity>(entity);

        velocity.velocity += gravity.force * dt;
    }
}
```

Example Main Loop:

```cpp
void main() {
    Coordinator coord;

    coord.init(); // Initializes entity manager, system manager and component manager
    coordinator.register_component<Gravity>();
    coordinator.register_component<Velocity>();

    auto physics_system = coordinator.register_system<physics_system>();

    Signature signature;
    signature.set(coord.get_component_type<Gravity>());
    signature.set(coord.get_component_type<Velocity>());

    coordinator.set_system_signature<PhysicsSystem>(signature); // Identify which components are going to be used in the system

    std::vector<Entity> entities (MAX_ENTITIES);

    for (auto& entity : entities) {
        entity = coord.create_entity();
        coord.add_component(entity, Gravity { glm::vec3(0.0f, -9.81f, 0.0f) });
        coord.add_component(entity, Velocity { glm::vec3(0.0f, 0.0f, 0.0f) });
    }

    float dt = 0.0f;

    while(!quit) {
        auto start_time = std::chrono::high_resolution_clock::now();
        physics_system->update(dt);
        auto stop_time = std::chrono::high_resolution_clock::now();
        dt = std::chrono::duration<float, std::chrono::seconds::period>(stop_time - start_time).count();
    }
}
```

## Vulkan

All the rendering is done using Vulkan, the boilerplate by `VkBootstrap`, obj loading through `tinyobjloader` and texture loading through `stb_image`

Here is what's finished:
- [X] Initialization 
- [X] Graphics Pipeline
- [X] Meshes
- [X] Buffers, Shader I/O
- [X] Textures
- [ ] Integration with ECS
