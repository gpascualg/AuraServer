#include "server/aura_server.hpp"
#include "entity/aura_entity.hpp"
#include "debug/debug.hpp"
#include "io/packet.hpp"
#include "map/cell.hpp"
#include "server/client.hpp"
#include "map/map.hpp"
#include "map/map-cluster/cluster.hpp"
#include "map/map_aware_entity.hpp"
#include "map/quadtree.hpp"
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
    _handlers.emplace(PacketOpcodes::FIRE_PACKET,       OpcodeHandler { &AuraServer::handleFire,            HandlerType::NORMAL });
    _handlers.emplace(PacketOpcodes::PING,              OpcodeHandler { nullptr,                            HandlerType::NO_CALLBACK });
    _handlers.emplace(PacketOpcodes::DISCONNECTION,     OpcodeHandler { nullptr,                            HandlerType::NO_CALLBACK });


    while (1)
    {
        update();
        diff = std::chrono::duration_cast<TimeBase>(now() - lastUpdate);
        lastUpdate = now();

        // First update map
        map()->update(diff.count());

        // Read packets
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

        // Consume scheduled tasks
        _nextTick.consume_all([this](auto func) 
            {
                func(this);
            }
        );

        // Last, server wide operations (ie. accept/close clients)
        runScheduledOperations();

        // Now cleanup map
        map()->cleanup(diff.count());

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

        LOG(LOG_SERVER_LOOP, "DIFF: %" PRId64 " - SLEEP: %" PRId64, diff.count(), prevSleepTime.count());
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
    auto entity = client->entity();

    // Log something
    LOG(LOG_FIRE_LOGIC, "Entity %" PRId64 " fired", entity->id());

    // TODO(gpascualg): Check if it can really fire
    // Broadcast fire packet
    Packet* broadcast = Packet::create((uint16_t)PacketOpcodes::FIRE_CANNONS_RESP);
    *broadcast << client->id() << packet;
    Server::map()->broadcastToSiblings(entity->cell(), broadcast);

    WeaponType type = (WeaponType)packet->read<uint8_t>();
    switch (type)
    {
        case WeaponType::CANNON:
            handleCanonFire(client, packet);
            break;

        case WeaponType::MORTAR:
            handleMortarFire(client, packet);
            break;

        default:
            // TODO(gpascualg): Disconnect client :D
            break;
    }
}

void AuraServer::handleCanonFire(Client* client, Packet* packet)
{
    auto entity = client->entity();
    auto motionMaster = entity->motionMaster();
    auto forward = motionMaster->forward();

    // Read which side shoots to
    uint8_t side = packet->read<uint8_t>();

    // Default to left side
    const auto& position2D = entity->motionMaster()->position2D();
    glm::vec2 fire_direction = { -forward.z, forward.x };
    glm::vec2 forward2D = { forward.x, forward.z };
    if (side == 1) 
    {
        fire_direction *= -1;
    }

    LOG(LOG_FIRE_LOGIC_EXT, " + Placed at (%f , %f)", position2D.x, position2D.y);

    // TODO(gpascualg): Fetch real number of canons and separation
    // Assume we have 5 canons, each at 0.1 of the other
    for (int i = -2; i <= 2; ++i)
    {
        glm::vec2 start = position2D + i * 1.0f * forward2D;
        glm::vec2 end = position2D + i * 1.0f * forward2D + 50.0f * fire_direction;

        LOG(LOG_FIRE_LOGIC_EXT, "    + Firing from (%f , %f) to (%f , %f)", start.x, start.y, end.x, end.y);

        // Check all candidates
        auto qt = entity->cell()->quadtree();
        std::list<MapAwareEntity*> entities;
        qt->trace(entities, start, end);

        float minDist = 0;
        MapAwareEntity* minEnt = nullptr;

        LOG(LOG_FIRE_LOGIC_EXT, "      + Number of candidates %" PRIuPTR, entities.size());

        for (auto*& candidate : entities)
        {
            if (candidate == entity)
            {
                continue;
            }

            float tmp;
            if (candidate->boundingBox()->intersects(start, end, &tmp))
            {
                if (!minEnt || tmp < minDist)
                {
                    minDist = tmp;
                    minEnt = candidate;
                }
            }
        }

        if (minEnt)
        {
            LOG(LOG_FIRE_LOGIC, "Hit %" PRId64 " at (%f, %f)", minEnt->id(), minEnt->motionMaster()->position().x, minEnt->motionMaster()->position().z);

            // TODO(gpascualg): This should aggregate number of hits per target, and set correct id
            Packet* broadcast = Packet::create((uint16_t)PacketOpcodes::FIRE_HIT);
            *broadcast << minEnt->id() << 1;
            Server::map()->broadcastToSiblings(client->entity()->cell(), broadcast);

            // TODO(gpascualg): Dynamic damage based on dist/weapon/etc
            // Apply damage
            static_cast<Entity*>(minEnt)->damage(50);
        }
    }
}

void AuraServer::handleMortarFire(Client* client, Packet* packet)
{
    auto entity = client->entity();
    auto motionMaster = entity->motionMaster();
    auto forward = motionMaster->forward();

    // Read direction & radius
    glm::vec2 direction = packet->read<glm::vec2>();
    float radius = packet->read<float>();

    // TODO(gpascualg): Check maximum radius

    // Default to left side
    const auto& position2D = entity->motionMaster()->position2D();
    LOG(LOG_FIRE_LOGIC_EXT, " + Placed at (%f , %f)", position2D.x, position2D.y);

    // Calculate hit point and create a bounding box there
    glm::vec2 hitPoint = position2D + direction * radius;
    BoundingBox* box = new CircularBoundingBox(hitPoint, { 0, 0, 0 }, radius);

    LOG(LOG_FIRE_LOGIC_EXT, "    + Firing to (%f , %f)", hitPoint.x, hitPoint.y);

    // TODO(gpascualg): This might not be the best place?
    auto qt = entity->cell()->quadtree();
    std::list<MapAwareEntity*> entities;
    qt->retrieve(entities, box->asRect());

    LOG(LOG_FIRE_LOGIC_EXT, "      + Number of candidates %" PRIuPTR, entities.size());

    for (auto*& candidate : entities)
    {
        if (candidate == entity)
        {
            continue;
        }

        if (SAT::get()->collides(candidate->boundingBox(), box))
        {
            LOG(LOG_FIRE_LOGIC, "Hit %" PRId64 " at (%f, %f)", candidate->id(), candidate->motionMaster()->position().x, candidate->motionMaster()->position().z);

            // TODO(gpascualg): This should aggregate number of hits per target, and set correct id
            Packet* broadcast = Packet::create((uint16_t)PacketOpcodes::FIRE_HIT);
            *broadcast << candidate->id() << 1;
            Server::map()->broadcastToSiblings(client->entity()->cell(), broadcast);

            // TODO(gpascualg): Dynamic damage based on dist/weapon/etc
            // Apply damage
            static_cast<Entity*>(candidate)->damage(50);
        }
    }
}

void AuraServer::handleAccept(Client* client, const boost::system::error_code& error)
{
    LOG(LOG_CLIENT_LIFECYCLE, "Client setup");

    // Setup entity
    _clients.emplace(client->id(), client);

    // TODO(gpascualg): Fetch position from DB
    auto motionMaster = client->entity()->motionMaster();
    motionMaster->teleport({ client->id(), 0, 0 });
    //LOG(LOG_DEBUG, "Entity spawning at %.2f %.2f", motionMaster->position().x, 0);

    // TODO(gpascualg): Use real bounding box sizes
    client->entity()->setupBoundingBox({ {-4.28, -16}, {-4.28, 14.77}, {4.28, 15.77}, {4.28, -16} });

    map()->addTo(client->entity(), nullptr);

    // TODO(gpascualg): Fetch from DB
    // TODO(gpascualg): Move out of here
    _nextTick.push([](AuraServer* server)
    {
        for (int i = 0; i < 1; ++i)
        {
            // TODO(gpascualg): Move this out to somewhere else
            static std::default_random_engine randomEngine;
            static std::uniform_real_distribution<> forwardDist(-1, 1); // rage 0 - 1

            MapAwareEntity* entity = server->newMapAwareEntity(AtomicAutoIncrement<0>::get(), nullptr);
            entity->setupBoundingBox({ {-4.28, -16}, {-4.28, 14.77}, {4.28, 15.77}, {4.28, -16} });
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
    client->scheduleRead(2);
    Server::handleAccept(client, error);
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
