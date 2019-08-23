#pragma once
#include "ecs.hpp"
#include "component-array.hpp"
#include <unordered_map>

class ComponentManager {
    public:
        template<typename T>
        void register_component() {
            const char* type_name = typeid(T).name();
            assert(component_types.find(type_name) == component_types.end() && "Component types registered more than once.");
            component_types.insert({ type_name, next_component_type });
            component_arrays.insert({ type_name, std::make_shared<ComponentArray<T>>()});
            next_component_type++;
        }

        template<typename T>
        ComponentType get_component_type() {
            const char* type_name = typeid(T).name();
            assert(component_types.find(type_name) != component_types.end() && "Component not registered");
            return component_types[type_name];
        }

        template<typename T>
        void add_component(Entity entity, T component) {
            get_component_array<T>()->insert_data(entity, component);
        }

        template<typename T>
        void remove_component(Entity entity) {
            get_component_array<T>()->remove_data(entity);
        }

        template<typename T>
        T& get_component(Entity entity) {
            return get_component_array<T>->get_data(entity);
        }

        void entity_destroyed(Entity entity) {
            for (auto const& pair : component_arrays) {
                auto const& component = pair.second;
                component->entity_destroyed(entity);
            }
        }

        template<typename T>
        std::shared_ptr<ComponentArray<T>> get_component_array() {
            const char* type_name = typeid(T).name();
            return std::static_pointer_cast<ComponentArray<T>>(component_arrays[type_name]);
        }

        std::unordered_map<const char*, ComponentType> component_types;
        std::unordered_map<const char*, std::shared_ptr<IComponentArray>> component_arrays;
        ComponentType next_component_type;
};
