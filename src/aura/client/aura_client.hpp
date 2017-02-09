#pragma once

#include "client.hpp"
#include "map_aware_entity.hpp"

#include <boost/pool/object_pool.hpp>


class Entity : public MapAwareEntity
{
public:
    using MapAwareEntity::MapAwareEntity;

    void onAdded(Cell* cell, Cell* old) override;
    void onRemoved(Cell* cell, Cell* to) override;
};

class AuraClient : public Client
{
public:
    using Client::Client;
};
