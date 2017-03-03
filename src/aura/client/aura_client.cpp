#include "aura_client.hpp"
#include "cell.hpp"
#include "debug.hpp"
#include "map.hpp"
#include "map_aware_entity.hpp"
#include "motion_master.hpp"
#include "packet.hpp"
#include "server.hpp"
#include "opcodes.hpp"


Packet* Entity::spawnPacket()
{
    Packet* packet = Packet::create((uint16_t)PacketOpcodes::ENTITY_SPAWN);
    *packet << id() << uint8_t{ 0 };
    *packet << motionMaster()->position().x << motionMaster()->position().y;
    *packet << motionMaster()->speed();
    *packet << motionMaster()->forward();
    return packet;
}

Packet* Entity::despawnPacket()
{
    Packet* packet = Packet::create((uint16_t)PacketOpcodes::ENTITY_DESPAWN);
    *packet << id() << uint8_t{ 0 };
    return packet;
}
