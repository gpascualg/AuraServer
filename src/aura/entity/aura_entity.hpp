#pragma once

#include "server/client.hpp"
#include "map/map_aware_entity.hpp"

#include <boost/pool/object_pool.hpp>


class Packet;

enum class WeaponType
{
    CANNON = 0,
    MORTAR = 1
};

class Entity : public MapAwareEntity
{
public:
    using MapAwareEntity::MapAwareEntity;

    Packet* spawnPacket() override;
    Packet* despawnPacket() override;

    void damage(int amount);
    void die();

private:
    int _health = 100.0f;
    bool alive = true;
};
