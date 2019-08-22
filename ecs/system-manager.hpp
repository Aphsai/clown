#include "ecs.hpp"

class SystemManager {
    public:
        template<typename T>
        std::shared_ptr<T> register_system() {
            const char* type_name = typeid(T).name();
            assert(systems.find(type_name) == systems.end() && "Registering system more than once");
            auto system = std::make_shared<T>();
            systems.insert({ type_name, system });
            return system;
        }

        template<typename T>
        void set_signature(Signature signature) {
            const char* type_name = typeid(T).name();
            assert(systems.find(type_name) != systems.end() && "System used before registering");
            signatures.insert({ type_name, signature });
        }

        void entity_destroyed(Entity entity) {
            for (auto const& pair : systems) {
                auto const& system = pair.second;
                system->entities.erase(entity);
            }
        }

        void entity_signature_changed(Entity entity, Signature entity_signature) {
            for (auto const& pair : systems) {
                auto const& type = pair.first;
                auto const& system = pair.second;
                auto const& system_signature = signatures[type];

                if ((entity_signature && system_signature) == system_signature) {
                    system->entities.insert(entity);
                } else {
                    system->entities.erase(entity);
                }
            }
        }

        std::unordered_map<const char*, Signature> signatures;
        std::unordered_map<const char*, std::shared_ptr<System>> systems;
};
