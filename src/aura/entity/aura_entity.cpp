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
    Packet* packet = Packet::create((uint16_t)PacketOpcodes::ENTITY_SPAWN);
    *packet << id() << uint8_t{ 0 };
    *packet << motionMaster()->position();
    *packet << motionMaster()->forward();
    *packet << (uint8_t)(motionMaster()->speed() * 1000.0f);
    
    // Movement generator
    if (motionMaster()->generator() && motionMaster()->generator()->hasNext())
    {
        *packet << (uint8_t)1;
        *packet << motionMaster()->generator()->packet();
    }
    else
    {
        *packet << (uint8_t)0;
    }

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
    motionMaster()->stop(true);

    // TODO(gpascualg): Sent packet with stop instructions (not only speed)
    Packet* broadcast = Packet::create((uint16_t)PacketOpcodes::ENTITY_DIED);
    *broadcast << id();

    schedule([](MapAwareEntity* entity) {
        auto cell = entity->cell();
        cell->map()->removeFrom(cell, entity, nullptr);
        
        // TODO(gpascualg): What happens with the entity? It should be cleaned
        // HACK(gpascualg): CLEAN MEMORY!
    }, Server::get()->now() + std::chrono::seconds(5));
}
