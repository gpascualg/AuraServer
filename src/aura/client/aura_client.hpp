#pragma once

#include "client.hpp"
#include "map_aware_entity.hpp"

#include <boost/pool/object_pool.hpp>


class Entity : public MapAwareEntity
{
public:
    using MapAwareEntity::MapAwareEntity;
};

class AuraClient : public Client
{
public:
    using Client::Client;
};
