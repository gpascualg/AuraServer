#include "aura_client.hpp"
#include "cell.hpp"
#include "debug.hpp"
#include "map.hpp"
#include "map_aware_entity.hpp"
#include "motion_master.hpp"
#include "packet.hpp"
#include "server.hpp"


Packet* Entity::spawnPacket()
{
    Packet* packet = Packet::create(0x0102);
    *packet << id() << uint8_t{ 0 };
    *packet << motionMaster()->position().x << motionMaster()->position().y;
    return packet;
}

Packet* Entity::despawnPacket()
{
    Packet* packet = Packet::create(0x0103);
    *packet << id() << uint8_t{ 0 };
    return packet;
}
