#include "aura_client.hpp"
#include "cell.hpp"
#include "debug.hpp"
#include "map.hpp"
#include "map_aware_entity.hpp"
#include "motion_master.hpp"
#include "packet.hpp"
#include "server.hpp"

void Entity::onAdded(Cell* cell, Cell* old)
{
    MapAwareEntity::onAdded(cell, old);

    Packet* packet = Packet::create();
    *packet << uint16_t{ 0x0102 } << uint16_t{ 0x0011 } << id() << uint8_t{ 0 };
    *packet << motionMaster()->position().x << motionMaster()->position().y;

    if (!old)
    {
        Server::get()->map()->broadcastToSiblings(cell, packet);
    }
    else
    {
        Server::get()->map()->broadcastExcluding(cell, old, packet);
    }
}

void Entity::onRemoved(Cell* cell, Cell* to)
{
    MapAwareEntity::onRemoved(cell, to);

    Packet* packet = Packet::create();
    *packet << uint16_t{ 0x0103 } << uint16_t{ 0x0009 } << id() << uint8_t{ 0 };

    if (!to)
    {
        Server::get()->map()->broadcastToSiblings(cell, packet);
    }
    else
    {
        Server::get()->map()->broadcastExcluding(to, cell, packet);
    }
}
