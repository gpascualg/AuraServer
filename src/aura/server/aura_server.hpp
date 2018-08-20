#pragma once

#include "defs/common.hpp"
#include "defs/traits.hpp"
#include "defs/atomic_autoincrement.hpp"
#include "server/server.hpp"
#include "client/aura_client.hpp"
#include "entity/aura_entity.hpp"
#include "opcodes.hpp"

#include <boost/lockfree/queue.hpp>

#include <atomic>
#include <unordered_map>
#include <list>

class Client;
class MapAwareEntity;
class Packet;

#define MAKE_HANDLER(x) [this](AbstractWork* w) -> AbstractWork* { return this->x(static_cast<first_agument_t<decltype(&AuraServer::x)>>(w)); }


class AuraServer : public Server
{
public:
    using Server::Server;

    void handleAccept(Client* client, const boost::system::error_code& error) override;
    void handleRead(Client* client, const boost::system::error_code& error, size_t size) override;

    void mainloop();

    AbstractWork* handleLogin(ClientWork* work);
    AbstractWork* loginResult(FutureWork<bool>* work);

    AbstractWork* handleForwardChange(ClientWork* work);
    AbstractWork* handleSpeedChange(ClientWork* work);
    AbstractWork* handleFire(ClientWork* work);

    Client* newClient(boost::asio::io_service* service, uint64_t id) override;
    void destroyClient(Client* client) override;

    void onCellCreated(Cell* cell) override;
    AbstractWork* onCellLoaded(FutureWork<std::vector<Entity*>>* work);
    void onCellDestroyed(Cell* cell) override;

    MapAwareEntity* newMapAwareEntity(uint64_t id, Client* client) override;
    void destroyMapAwareEntity(MapAwareEntity* entity) override;
    void iterateClients(std::function<void(Client* client)> callback) override;

protected:
    void handleCanonFire(Client* client, Packet* packet);
    void handleMortarFire(Client* client, Packet* packet);

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
    boost::lockfree::queue<void(*)(AuraServer*), boost::lockfree::capacity<4096>> _syncQueue;

    enum class HandlerType
    {
        NO_CALLBACK,
        ASYNC_CLIENT,
        SYNC_SERVER
    };

    enum class Condition
    {
        NONE,
        NOT_IN_WORLD,
        ALIVE
    };

    struct OpcodeHandler
    {
        ExecutorWork work;
        HandlerType type;
        Condition cond;
    };

#if defined(_WIN32)
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
