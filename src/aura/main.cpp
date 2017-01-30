#include "cell.hpp"
#include "cluster.hpp"
#include "map.hpp"
#include "map_aware_entity.hpp"
#include "server.hpp"
#include "client.hpp"

#include <stdio.h>
#include <chrono>
#include <unordered_map>
#include <mutex>

#include <boost/pool/object_pool.hpp>
#include <boost/asio.hpp>


class LockedID
{
public:
    static uint64_t get()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return ++_last;
    }

private:
    static uint64_t _last;
    static std::mutex _mutex;
};
uint64_t LockedID::_last = 0;
std::mutex LockedID::_mutex;


class AuraClient : public Client
{
public:
    using Client::Client;

    inline void setId(uint64_t id) { _id = id; }
    inline uint64_t getId() { return _id; }

private:
    uint64_t _id;
};


class Entity : public MapAwareEntity
{
    uint32_t id() { return 0; }

    void onAdded(Cell* cell) {}
    void onRemoved(Cell* cell) {}
};

class AuraServer : public Server
{
public:
    using Server::Server;

    void handleAccept(Client* client, const boost::system::error_code& error)
    {
        printf("CLIENT ACCEPTED\n");
        AuraClient* auraClient = (AuraClient*)client;
        auraClient->setId(LockedID::get());
        _clients.emplace(auraClient->getId(), auraClient);

        Server::handleAccept(client, error);
        client->scheduleRead(1);
    }

    void handleRead(Client* client, const boost::system::error_code& error, size_t size)
    {
        if (!error)
        {
            client->data()[client->totalRead()] = '\0';
            printf("MSG (%d): %s\n", *error, client->data());
            client->scheduleRead(1);
        }
        else
        {
            client->socket()->close();
        }
    }

private:
    std::unordered_map<uint64_t, AuraClient*> _clients;
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
