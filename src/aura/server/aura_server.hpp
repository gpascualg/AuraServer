#pragma once

#include "server.hpp"
#include "atomic_autoincrement.hpp"

#include <boost/lockfree/queue.hpp>

#include <atomic>
#include <unordered_map>
#include <list>

class Client;
class Packet;

class AuraServer : public Server
{
public:
    using Server::Server;

    void handleAccept(Client* client, const boost::system::error_code& error) override;
    void handleRead(Client* client, const boost::system::error_code& error, size_t size) override;

    void mainloop();

protected:
    std::unordered_map<uint64_t, Client*> _clients;

    struct Recv
    {
        Packet* packet;
        Client* client;
    };
    boost::lockfree::queue<Recv, boost::lockfree::capacity<4096>> _packets;
};
