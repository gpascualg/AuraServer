#include "cell.hpp"
#include "cluster.hpp"
#include "map.hpp"
#include "map_aware_entity.hpp"
#include "server.hpp"
#include "client.hpp"

#include "aura_server.hpp"
#include "aura_client.hpp"

#include <stdio.h>
#include <chrono>
#include <unordered_map>
#include <mutex>

#include <boost/pool/object_pool.hpp>
#include <boost/asio.hpp>


class Entity : public MapAwareEntity
{
    uint32_t id() { return 0; }

    void onAdded(Cell* cell) override {}
    void onRemoved(Cell* cell) override {}
    void update(uint64_t elapsed) override {}
};

int main()
{
    typedef std::chrono::high_resolution_clock Time;
    typedef std::chrono::nanoseconds ns;

    Map map;
    AuraServer server(12345, (boost::object_pool<Client>*)new boost::object_pool<AuraClient>());

    server.startAccept();
    server.updateIO();

    return 0;
}
