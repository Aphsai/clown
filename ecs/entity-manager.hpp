#include "ecs.hpp"

class EntityManager {
    public:
        EntityManager();
        Entity create_entity();
        void destroy_entity(Entity entity);
        void set_signature(Entity entity, Signature signature);
        Signature get_signature(Entity entity);

        std::queue<Entity> available_entities;
        std::array<Signature, MAX_ENTITIES> signatures;
        uint32_t living_entity_count;
};
