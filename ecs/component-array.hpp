#include "ecs.hpp"
#include <unordered_map>
#include <array>

class IComponentArray {
    public:
        virtual ~IComponentArray() = default;
        virtual void entity_destroyed(Entity entity) = 0;
};

template<typename T>
class ComponentArray : public IComponentArray {
    public:
        void insert_data(Entity entity, T component) {
            assert(entity_to_index.find(entity) == entity_to_index.end() && "Component added to same entity");
            size_t new_index = size;
            entity_to_index[entity] = new_index;
            index_to_entity[new_index] = entity;
            component_array[new_index] = component;
        }

        void remove_data(Entity entity) {
            assert(entity_to_index.find(entity) == entity_to_index.end() && "Component added to same entity");
            size_t index_of_removed_entity = entity_to_index[entity];
            size_t index_of_last_element = size - 1;
            component_array[index_of_removed_entity] = component_array[index_of_last_element];

            Entity entity_of_last_element = index_to_entity[index_of_last_element];
            entity_to_index[entity_of_last_element] = index_of_removed_entity;
            index_to_entity[index_of_removed_entity] = entity_of_last_element;

            entity_to_index.erase(entity);
            index_to_entity.erase(index_of_last_element);
            size--;
        }

        T& get_data(Entity entity) {
            return component_array[entity_to_index[entity]];
        }

        void entity_destroyed(Entity entity) override {
            if (entity_to_index.find(entity) != entity_to_index.end()) remove_data(entity);
        }

        std::array<T, MAX_ENTITIES> component_array;
        std::unordered_map<Entity, size_t> entity_to_index;
        std::unordered_map<size_t, Entity> index_to_entity;
        size_t size;
}
