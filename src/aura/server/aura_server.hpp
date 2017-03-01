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


enum class PacketOpcodes
{
    ENTITY_SPAWN =                  0x0102,
    OPCODE_FORWARD_CHANGE =         0x0101,
    OPCODE_FORWARD_CHANGE_RESP =    0x0A01,
    OPCODE_SPEED_CHANGE =           0x0103,
    OPCODE_SPEED_CHANGE_RESP =      0x0A03,
    OPCODE_MOVEMENT =               0x0104,
    OPCODE_MOVEMENT_RESP =          0x0A04,
    OPCODE_CHAT =                   0x0011,
    OPCODE_SET_ID =                 0x0012,
    OPCODE_PING =                   0x0001,
    OPCODE_DISCONNECTION =          0x0002
};


class AuraServer : public Server
{
public:
    using Server::Server;

    void handleAccept(Client* client, const boost::system::error_code& error) override;
    void handleRead(Client* client, const boost::system::error_code& error, size_t size) override;

    void mainloop();

    void handleForwardChange(Client* client, Packet* packet);
    void handleMovement(Client* client, Packet* packet);
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

#if defined(_WIN32) || defined(__clang__)
    std::unordered_map<PacketOpcodes, void (AuraServer::*)(Client*, Packet*)> _handlers;
#else
    struct EnumClassHash
    {
        template <typename T>
        std::size_t operator()(T t) const
        {
            return static_cast<std::size_t>(t);
        }
    };

    std::unordered_map<PacketOpcodes, void (AuraServer::*)(Client*, Packet*), EnumClassHash> _handlers;
#endif
};
