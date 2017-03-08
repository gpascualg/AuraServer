#include "aura_client.hpp"
#include "cell.hpp"
#include "debug.hpp"
#include "map.hpp"
#include "map_aware_entity.hpp"
#include "motion_master.hpp"
#include "movement_generator.hpp"
#include "packet.hpp"
#include "server.hpp"
#include "opcodes.hpp"


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
