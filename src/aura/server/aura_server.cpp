#include "aura_server.hpp"
#include "debug.hpp"
#include "packet.hpp"
#include "cell.hpp"
#include "client.hpp"
#include "map.hpp"
#include "cluster.hpp"
#include "map_aware_entity.hpp"
#include "motion_master.hpp"

#include <inttypes.h>
#include <thread>

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>


using TimeBase = std::chrono::milliseconds;

const uint16_t MAX_PACKET_LEN = 1000;
const TimeBase WORLD_HEART_BEAT = TimeBase(50);

void AuraServer::mainloop()
{
    auto lastUpdate = std::chrono::high_resolution_clock::now();
    auto diff = lastUpdate - lastUpdate; // HACK(gpascualg): Cheaty way to get duration
    auto prevSleepTime = diff;

    _handlers.emplace(PacketOpcodes::OPCODE_SPEED_CHANGE,   &AuraServer::handleSpeedChange);
    _handlers.emplace(PacketOpcodes::OPCODE_FORWARD_CHANGE, &AuraServer::handleForwardChange);
    _handlers.emplace(PacketOpcodes::OPCODE_MOVEMENT,       &AuraServer::handleMovement);

    while (1)
    {
        auto now = std::chrono::high_resolution_clock::now();
        diff = now - lastUpdate;
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
                        //recv.client->close();
                    }
                    else
                    {
                        (this->*handler->second)(recv.client, recv.packet);
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
    glm::vec2 forward = { packet->read<float>(), packet->read<float>() };
    client->entity()->motionMaster()->forward(forward);

    Packet* broadcast = Packet::create(0x0A01);
    *broadcast << client->id();
    *broadcast << forward.x << forward.y;

    Server::map()->broadcastToSiblings(client->entity()->cell(), broadcast);
}

void AuraServer::handleMovement(Client* client, Packet* packet)
{
    bool isEnd = packet->read<bool>();
    if (isEnd)
    {
        client->entity()->motionMaster()->stop();
    }
    else
    {
        client->entity()->motionMaster()->move();
    }

    Packet* broadcast = Packet::create(0x0A04);
    *broadcast << client->id() << isEnd;

    Server::map()->broadcastToSiblings(client->entity()->cell(), broadcast);
}

void AuraServer::handleSpeedChange(Client* client, Packet* packet)
{
    uint8_t speed = packet->read<uint8_t>();
    client->entity()->motionMaster()->speed(speed);

    Packet* broadcast = Packet::create(0x0A03);
    *broadcast << client->id() << speed;

    Server::map()->broadcastToSiblings(client->entity()->cell(), broadcast);
}

void AuraServer::handleAccept(Client* client, const boost::system::error_code& error)
{
    // Setup entity
    _clients.emplace(client->id(), client);

    // TODO(gpascualg): Fetch position from DB
    auto motionMaster = client->entity()->motionMaster();
    motionMaster->teleport({ client->id(), client->id() });
    LOG(LOG_DEBUG, "Entity spawning at %.0f %.0f", motionMaster->position().x, motionMaster->position().y);

    map()->addTo(client->entity(), nullptr);

    // Send ID
    Packet* packet = Packet::create(0x0012);
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
