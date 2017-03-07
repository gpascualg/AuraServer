#pragma once

#include "server.hpp"
#include "atomic_autoincrement.hpp"
#include "aura_client.hpp"
#include "opcodes.hpp"

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

    void handleForwardChange(Client* client, Packet* packet);
    void handleSpeedChange(Client* client, Packet* packet);

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
    boost::lockfree::queue<void(*)(AuraServer*), boost::lockfree::capacity<4096>> _nextTick;

    enum class HandlerType
    {
        NO_CALLBACK,
        NORMAL
    };

    struct OpcodeHandler
    {
        void (AuraServer::*callback)(Client*, Packet*);
        HandlerType type;
    };

#if defined(_WIN32) || defined(__clang__)
    std::unordered_map<PacketOpcodes, OpcodeHandler> _handlers;
#else
    struct EnumClassHash
    {
        template <typename T>
        std::size_t operator()(T t) const
        {
            return static_cast<std::size_t>(t);
        }
    };

    std::unordered_map<PacketOpcodes, OpcodeHandler, EnumClassHash> _handlers;
#endif
};
