#include "entity/aura_entity.hpp"
#include "map/cell.hpp"
#include "debug/debug.hpp"
#include "map/map.hpp"
#include "map/map_aware_entity.hpp"
#include "movement/motion_master.hpp"
#include "movement/movement_generator.hpp"
#include "io/packet.hpp"
#include "server/server.hpp"
#include "server/opcodes.hpp"


Packet* Entity::spawnPacket()
{
    auto& trans = transform();

    Packet* packet = Packet::create((uint16_t)PacketOpcodes::ENTITY_SPAWN);
    *packet << id() << uint8_t{ 0 };
    *packet << trans.Position;
    *packet << trans.Forward;
    *packet << (uint8_t)(trans.Speed * 1000.0f);

    // Movement generator
    // TODO(gpascualg): Recode when movement generators are implemented
    *packet << (uint8_t)0;
    // if (motionMaster()->generator() && motionMaster()->generator()->hasNext())
    // {
    //     *packet << (uint8_t)1;
    //     *packet << motionMaster()->generator()->packet();
    // }
    // else
    // {
    //     *packet << (uint8_t)0;
    // }

    return packet;
}

Packet* Entity::despawnPacket()
{
    Packet* packet = Packet::create((uint16_t)PacketOpcodes::ENTITY_DESPAWN);
    *packet << id() << uint8_t{ 0 };
    return packet;
}

void Entity::damage(float amount)
{
    _health -= amount;
    if (_health <= 0 && _alive)
    {
        _alive = false;
        die();
    }
}

void Entity::die()
{
    // Stop any movement
    _transform.stop(Server::get()->now());

    Packet* broadcast = Packet::create((uint16_t)PacketOpcodes::ENTITY_DIED);
    *broadcast << id();

    schedule([](Executor<ExecutorQueueMax>* executor) {
        auto entity = static_cast<MapAwareEntity*>(executor);
        auto cell = entity->cell();
        cell->map()->removeFrom(cell, entity, nullptr);

        // TODO(gpascualg): What happens with the entity? It should be cleaned
        // HACK(gpascualg): CLEAN MEMORY!
    }, Server::get()->now() + std::chrono::seconds(5));
}
