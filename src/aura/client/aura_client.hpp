#pragma once

#include "server/client.hpp"
#include "map/map_aware_entity.hpp"

#include <boost/pool/object_pool.hpp>


class Packet;

class Entity : public MapAwareEntity
{
public:
    using MapAwareEntity::MapAwareEntity;

    Packet* spawnPacket() override;
    Packet* despawnPacket() override;
};

class AuraClient : public Client
{
public:
    using Client::Client;
};
