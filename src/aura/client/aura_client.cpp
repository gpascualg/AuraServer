#include "aura_client.hpp"
#include "cell.hpp"
#include "debug.hpp"
#include "map_aware_entity.hpp"
#include "packet.hpp"

void Entity::onAdded(Cell* cell)
{
    MapAwareEntity::onAdded(cell);

    Packet* packet = Packet::create();
    *packet << uint16_t{ 0x0102 } << uint16_t{ 0x0009 } << client()->id() << uint8_t{ 0 };
    cell->broadcast(packet, true);
}
