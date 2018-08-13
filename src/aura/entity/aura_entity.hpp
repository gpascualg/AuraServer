#pragma once

#include "server/client.hpp"
#include "map/map_aware_entity.hpp"

#include <boost/pool/object_pool.hpp>
#include <bsoncxx/types.hpp>


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

    void damage(float amount);
    void die();

    inline bool isAlive() { return _alive; };

    inline void mongoId(bsoncxx::types::b_oid mongoId) { _mongoId = mongoId; }
    inline const bsoncxx::types::b_oid& mongoId() { return _mongoId; }

private:
    bsoncxx::types::b_oid _mongoId;

    float _health = 100.0f;
    bool _alive = true;
};
