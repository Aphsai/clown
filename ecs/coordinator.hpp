#pragma once
#include "ecs.hpp"
#include "system_manager.hpp"
#include "entity_manager.hpp"
#include "component_manager.hpp"

class Coordinator {
    public:
        inline void init() {
            component_manager = std::make_unique<ComponentManager>();
            entity_manager = std::make_unique<EntityManager>();
            system_manager = std::make_unique<SystemManager>();
        }

        inline void destroy_entity(Entity entity) {
            entity_manager->destroy_entity(entity);
            component_manager->entity_destroyed(entity);
            system_manager->entity_destroyed(entity);
        }

        inline Entity create_entity() {
            return entity_manager->create_entity();
        }

        template<typename T> 
        inline void register_component() {
            component_manager->register_component<T>();
        }

        template<typename T>
        inline void add_component(Entity entity, T component) {
            component_manager->add_component<T>(entity, component);
            auto signature = entity_manager->get_signature(entity);
            signature.set(component_manager->get_component_type<T>(), true);
            entity_manager->set_signature(entity, signature);
            system_manager->entity_signature_changed(entity, signature);
        }

        template<typename T>
        inline void remove_component(Entity entity) {
            component_manager->remove_component<T>(entity);
            auto signature = entity_manager->get_signature(entity);
            signature.set(component_manager->get_component_type<T>(), false);
            entity_manager->set_signature(entity, signature);
            system_manager->entity_signature_changed(entity, signature);
        }

        template<typename T>
        inline T& get_component(Entity entity) {
            return component_manager->get_component<T>(entity);
        }

        template<typename T>
        inline ComponentType get_component_type() {
            return component_manager->get_component_type<T>();
        }

        template<typename T>
        inline std::shared_ptr<T> register_system() {
            return system_manager->register_system<T>();
        }

        template<typename T>
        inline void set_system_signature(Signature signature) {
            system_manager->set_signature<T>(signature);
        }

        std::unique_ptr<ComponentManager> component_manager;
        std::unique_ptr<EntityManager> entity_manager;
        std::unique_ptr<SystemManager> system_manager;
};
