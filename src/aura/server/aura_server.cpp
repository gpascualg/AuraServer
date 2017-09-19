#include "server/aura_server.hpp"
#include "debug/debug.hpp"
#include "io/packet.hpp"
#include "map/cell.hpp"
#include "server/client.hpp"
#include "map/map.hpp"
#include "map/map-cluster/cluster.hpp"
#include "map/map_aware_entity.hpp"
#include "movement/motion_master.hpp"
#include "movement/movement_generator.hpp"

#include <inttypes.h>
#include <random>
#include <thread>

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>


using TimeBase = std::chrono::milliseconds;

const uint16_t MAX_PACKET_LEN = 1000;
const TimeBase WORLD_HEART_BEAT = TimeBase(50);

void AuraServer::mainloop()
{
    auto lastUpdate = std::chrono::high_resolution_clock::now();
    auto diff = std::chrono::duration_cast<TimeBase>(lastUpdate - lastUpdate); // HACK(gpascualg): Cheaty way to get duration
    auto prevSleepTime = std::chrono::duration_cast<TimeBase>(diff);


    _handlers.emplace(PacketOpcodes::SPEED_CHANGE,      OpcodeHandler { &AuraServer::handleSpeedChange,     HandlerType::NORMAL });
    _handlers.emplace(PacketOpcodes::FORWARD_CHANGE,    OpcodeHandler { &AuraServer::handleForwardChange,   HandlerType::NORMAL });
    _handlers.emplace(PacketOpcodes::FIRE_CANNONS,      OpcodeHandler { &AuraServer::handleFire,            HandlerType::NORMAL });
    _handlers.emplace(PacketOpcodes::PING,              OpcodeHandler { nullptr,                            HandlerType::NO_CALLBACK });
    _handlers.emplace(PacketOpcodes::DISCONNECTION,     OpcodeHandler { nullptr,                            HandlerType::NO_CALLBACK });


    while (1)
    {
        auto now = std::chrono::high_resolution_clock::now();
        diff = std::chrono::duration_cast<TimeBase>(now - lastUpdate);
        lastUpdate = now;

        _packets.consume_all([this](auto recv)
            {
                if (recv.client->inMap())
                {
                    // Read packet
                    Packet* packet = recv.packet;
                    uint16_t opcode = packet->read<uint16_t>();
                    uint16_t len = packet->read<uint16_t>();

                    auto handler = _handlers.find((PacketOpcodes)opcode);
                    if (handler == _handlers.end())
                    {
                        recv.client->close();
                    }
                    else
                    {
                        auto& opcodeHandler = handler->second;
                        if (opcodeHandler.type != HandlerType::NO_CALLBACK)
                        {
                            (this->*opcodeHandler.callback)(recv.client, recv.packet);
                        }
                    }


                    // Back to the pool
                    recv.packet->destroy();
                }
                else
                {
                    LOG(LOG_FATAL, "Should not happen to have a broadcast while not in map");
                }
            }
        );

        map()->update(diff.count());
        runScheduledOperations();

        _nextTick.consume_all([this](auto func) 
            {
                func(this);
            }
        );

        // Wait for a constant update time
        if (diff <= WORLD_HEART_BEAT + prevSleepTime)
        {
            prevSleepTime = WORLD_HEART_BEAT + prevSleepTime - diff;
            std::this_thread::sleep_for(prevSleepTime);
        }
        else
        {
            prevSleepTime = prevSleepTime.zero();
        }

        LOG(LOG_SERVER_LOOP, "DIFF: %" PRId64 " - SLEEP: %" PRId64, diff, prevSleepTime);
    }
}

void AuraServer::handleForwardChange(Client* client, Packet* packet)
{
    float speed = packet->read<float>();
    auto motionMaster = client->entity()->motionMaster();
    motionMaster->forward(speed);

    Packet* broadcast = Packet::create((uint16_t)PacketOpcodes::FORWARD_CHANGE_RESP);
    *broadcast << client->id();
    *broadcast << speed;

    if (speed == 0)
    {
        *broadcast << motionMaster->forward();
    }

    Server::map()->broadcastToSiblings(client->entity()->cell(), broadcast);
}

void AuraServer::handleSpeedChange(Client* client, Packet* packet)
{
    auto motionMaster = client->entity()->motionMaster();
    int8_t speed = packet->read<int8_t>();
    
    Packet* broadcast = Packet::create((uint16_t)PacketOpcodes::SPEED_CHANGE_RESP);
    *broadcast << client->id() << speed;
    *broadcast << motionMaster->position();

    motionMaster->speed(speed);

    if (speed != 0)
    {
        motionMaster->move();
    }
    else
    {
        motionMaster->stop();
    }

    Server::map()->broadcastToSiblings(client->entity()->cell(), broadcast);
}

void AuraServer::handleFire(Client* client, Packet* packet)
{
    auto motionMaster = client->entity()->motionMaster();
    auto forward = motionMaster->forward();

    uint8_t side = packet->read<int8_t>();

    // TODO(gpascualg): Check if it can really fire
    Packet* broadcast = Packet::create((uint16_t)PacketOpcodes::FIRE_CANNONS_RESP);
    *broadcast << client->id() << side;
    Server::map()->broadcastToSiblings(client->entity()->cell(), broadcast);

    // Default to right side
    glm::vec3 fire_direction = { -forward.y, 0, forward.x };
    if (side == 0) 
    {
        fire_direction *= -1;
    }

    // TODO(gpascualg): Fetch real number of canons and separation
    // Assume we have 5 canons, each at 0.1 of the other
    for (int i = -2; i <= 2; ++i)
    {
        glm::vec2 start = fire_direction + 0.1f * forward;
        float reach = 1.0f;

        // TODO(gpascualg): Throw ray and test collision
    }

    // TODO(gpascualg): This should aggregate number of hits per target, and set correct id
    broadcast = Packet::create((uint16_t)PacketOpcodes::FIRE_HIT);
    *broadcast << client->id() << 1;
    Server::map()->broadcastToSiblings(client->entity()->cell(), broadcast);
}

void AuraServer::handleAccept(Client* client, const boost::system::error_code& error)
{
    // Setup entity
    _clients.emplace(client->id(), client);

    // TODO(gpascualg): Fetch position from DB
    auto motionMaster = client->entity()->motionMaster();
    motionMaster->teleport({ client->id(), 0, 0 });
    //LOG(LOG_DEBUG, "Entity spawning at %.2f %.2f", motionMaster->position().x, 0);

    map()->addTo(client->entity(), nullptr);

    // TODO(gpascualg): Fetch from DB
    // TODO(gpascualg): Move out of here
    _nextTick.push([](AuraServer* server)
    {
        for (int i = 0; i < 3; ++i)
        {
            // TODO(gpascualg): Move this out to somewhere else
            static std::default_random_engine randomEngine;
            static std::uniform_real_distribution<> forwardDist(-1, 1); // rage 0 - 1

            MapAwareEntity* entity = server->newMapAwareEntity(AtomicAutoIncrement<0>::get(), nullptr);
            entity->motionMaster()->teleport({ 0,0,0 });
            entity->motionMaster()->forward(glm::normalize(glm::vec3{ forwardDist(randomEngine), 0, forwardDist(randomEngine) }));
            entity->motionMaster()->generator(new RandomMovement());
            server->map()->addTo(entity, nullptr);
        }
    });

    // Send ID
    Packet* packet = Packet::create((uint16_t)PacketOpcodes::SET_ID);
    *packet << client->id();
    client->send(packet);

    // Start receiving!
    Server::handleAccept(client, error);
    client->scheduleRead(2);
}

void AuraServer::handleRead(Client* client, const boost::system::error_code& error, size_t size)
{
#if IF_LOG(LOG_PACKET_RECV)
    if (!error)
    {
        printf("Recv bytes at [%" PRId64 "] - Size: %d\n", time(NULL), size);
        for (uint16_t i = 0; i < client->packet()->size(); ++i)
        {
            printf("%.2X ", (uint8_t)client->packet()->data()[i]);
        }
        printf("\n");
    }
#endif

    bool disconnect = false;
    uint16_t len = 2;
    bool reset = false;

    switch (client->readPhase())
    {
        // Opcode
        case 1:
            break;

        // Length
        case 2:
            len = client->packet()->peek<uint16_t>(2);
            if (len >= MAX_PACKET_LEN)
            {
                disconnect = true;
            }

            // No data in packet
            reset = len == 0;
            break;

        case 3:
            reset = true;
            break;
    }

    if (!disconnect)
    {
        if (reset)
        {
            // Force read 2 bytes now (opcode)
            len = 2;

            LOG(LOG_PACKET_RECV, "[PACKET DONE]");

            _packets.push({
                Packet::copy(client->packet(), client->packet()->size()),
                client
            });
        }

        client->scheduleRead(len, reset);
    }
    else
    {
        client->close();
    }
}

Client* AuraServer::newClient(boost::asio::io_service* service, uint64_t id)
{
    return static_cast<Client*>(_clientPool.construct(service, id));
}

void AuraServer::destroyClient(Client* client)
{
    _clientPool.destroy(static_cast<AuraClient*>(client));
}

MapAwareEntity* AuraServer::newMapAwareEntity(uint64_t id, Client* client)
{
    return static_cast<MapAwareEntity*>(_entityPool.construct(id, client));
}

void AuraServer::destroyMapAwareEntity(MapAwareEntity* entity)
{
    _entityPool.destroy(static_cast<Entity*>(entity));
}
