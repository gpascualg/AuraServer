#pragma once

#include "server.hpp"
#include "atomic_autoincrement.hpp"
#include "aura_client.hpp"

#include <boost/lockfree/queue.hpp>

#include <atomic>
#include <unordered_map>
#include <list>

class Client;
class MapAwareEntity;
class Packet;

class AuraServer : public Server
{
public:
    using Server::Server;

    void handleAccept(Client* client, const boost::system::error_code& error) override;
    void handleRead(Client* client, const boost::system::error_code& error, size_t size) override;

    void mainloop();

    Client* newClient(boost::asio::io_service* service, uint64_t id) override;
    void destroyClient(Client* client) override;

    MapAwareEntity* newMapAwareEntity(uint64_t id, Client* client) override;
    void destroyMapAwareEntity(MapAwareEntity* entity) override;

protected:
    std::unordered_map<uint64_t, Client*> _clients;
    boost::object_pool<AuraClient> _clientPool;
    boost::object_pool<Entity> _entityPool;

    struct Recv
    {
        Packet* packet;
        Client* client;
    };
    boost::lockfree::queue<Recv, boost::lockfree::capacity<4096>> _packets;
};
