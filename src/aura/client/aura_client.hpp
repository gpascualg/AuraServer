#pragma once

#include "server/client.hpp"
#include "map/map_aware_entity.hpp"

#include <boost/pool/object_pool.hpp>


class Packet;

class AuraClient : public Client
{
public:
    using Client::Client;
};
