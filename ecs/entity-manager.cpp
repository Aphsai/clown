#include "entity-manager.hpp"
#include <assert.h>

EntityManager::EntityManager() {
    for (Entity entity = 0; entity < MAX_ENTITIES; entity++) {
        available_entities.push(entity);
    }
}

Entity EntityManager::create_entity() {
    assert(living_entity_count < MAX_ENTITIES && "Too many entities exist.");
    Entity id = available_entities.front();
    available_entities.pop();
    living_entity_count++;
    return id;
}

void EntityManager::destroy_entity(Entity entity) {
    assert(entity < MAX_ENTITIES && "Entity out of range.");
    signatures[entity].reset();
    available_entities.push(entity);
    living_entity_count--;
}

void EntityManager::set_signature(Entity entity, Signature signature) {
    assert(entity < MAX_ENTITIES && "Entity out of range.");
    signatures[entity] = signature;
}

Signature EntityManager::get_signature(Entity entity) {
    assert(entity < MAX_ENTITIES && "Entity out of range.");
    return signatures[entity];
}




