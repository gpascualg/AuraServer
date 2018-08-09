#include "server/aura_server.hpp"
#include "entity/aura_entity.hpp"
#include "database/database.hpp"
#include "debug/debug.hpp"
#include "framework/framework.hpp"
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

template <typename> struct Types;

template <typename R, typename A, typename... Args>
struct Types<R(AuraServer::*)(A, Args...)>
{
    using returnType = R;
    using firstArgType = A;
};

template <typename T> using first_agument_t = typename Types<T>::firstArgType;
template <typename T> using return_t = typename Types<T>::returnType;


#define MAKE_HANDLER(x) [this](AbstractWork* w) -> AbstractWork* { return this->x(static_cast<first_agument_t<decltype(&AuraServer::x)>>(w)); }
// #define MALE_HANDLER(x) std::bind(&AuraServer::x, this, std::placeholders::_1)

void AuraServer::mainloop()
{
    _handlers.emplace(PacketOpcodes::CLIENT_LOGIN,      OpcodeHandler { MAKE_HANDLER(handleLogin),          HandlerType::ASYNC_CLIENT,  Condition::NONE });
    _handlers.emplace(PacketOpcodes::SPEED_CHANGE,      OpcodeHandler { MAKE_HANDLER(handleSpeedChange),    HandlerType::ASYNC_CLIENT,  Condition::ALIVE });
    _handlers.emplace(PacketOpcodes::FORWARD_CHANGE,    OpcodeHandler { MAKE_HANDLER(handleForwardChange),  HandlerType::ASYNC_CLIENT,  Condition::ALIVE });
    _handlers.emplace(PacketOpcodes::FIRE_PACKET,       OpcodeHandler { MAKE_HANDLER(handleFire),           HandlerType::ASYNC_CLIENT,  Condition::ALIVE });
    _handlers.emplace(PacketOpcodes::PING,              OpcodeHandler { nullptr,                            HandlerType::NO_CALLBACK,   Condition::NONE });
    _handlers.emplace(PacketOpcodes::DISCONNECTION,     OpcodeHandler { nullptr,                            HandlerType::NO_CALLBACK,   Condition::NONE });

    while (1)
    {
        update();

        // TODO: Smarter debugging of clients, this is costy!
        Reactive::get()->clients.clear();
        for (auto pair : _clients)
        {
            Reactive::get()->clients.push_back(pair.second);
        }
    }
}

AbstractWork* AuraServer::handleLogin(ClientWork* work)
{
    std::string username = work->packet()->read<std::string>();
    std::string password = work->packet()->read<std::string>();

    auto future = Framework::get()->database()->query<bool>("aura", [username, password](const mongocxx::database& db) {
        bsoncxx::builder::stream::document filter_builder;
        filter_builder << "_id" << username << "password" << password;

        return db["users"].count(filter_builder.view()) == 1;
    });

    return new FutureWork<bool>(MAKE_HANDLER(loginResult), work->executor(), std::move(future));
}

AbstractWork* AuraServer::loginResult(FutureWork<bool>* work)
{
    Packet* packet = Packet::create((uint16_t)PacketOpcodes::CLIENT_LOGIN_RESP);
    *packet << (uint8_t)work->get();
    work->executor()->send(packet);

    return nullptr;
}

AbstractWork* AuraServer::handleForwardChange(ClientWork* work)
{
    Client* client = work->executor();
    Packet* packet = work->packet();

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
    return nullptr;
}

AbstractWork* AuraServer::handleSpeedChange(ClientWork* work)
{
    Client* client = work->executor();
    Packet* packet = work->packet();

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
    return nullptr;
}

AbstractWork* AuraServer::handleFire(ClientWork* work)
{
    Client* client = work->executor();
    Packet* packet = work->packet();
    
    auto entity = client->entity();

    // TODO(gpascualg): Check if it can really fire
    // Broadcast fire packet
    Packet* broadcast = Packet::create((uint16_t)PacketOpcodes::FIRE_CANNONS_RESP);
    *broadcast << client->id() << packet;
    Server::map()->broadcastToSiblings(entity->cell(), broadcast);

    WeaponType type = (WeaponType)packet->read<uint8_t>();

    // Log something
    LOG(LOG_FIRE_LOGIC, "Entity %" PRId64 " fired [Type: %d]", entity->id(), static_cast<uint8_t>(type));

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
            return nullptr;
            break;
    }

    return nullptr;
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

    LOG(LOG_FIRE_LOGIC, " + Placed at (%f , %f) with direction %d", position2D.x, position2D.y, side);

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
            *broadcast << minEnt->id() << static_cast<uint8_t>(WeaponType::CANNON) << 50.0f;
            Server::map()->broadcastToSiblings(client->entity()->cell(), broadcast);

            // TODO(gpascualg): Dynamic damage based on dist/weapon/etc
            // Apply damage
            static_cast<Entity*>(minEnt)->damage(50.0f);
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
    glm::vec2 hitPoint2D = position2D + direction * radius;
    glm::vec3 hitPoint { hitPoint2D.x, 0, hitPoint2D.y };
    CircularBoundingBox box(hitPoint, { 0, 0, 0 }, radius);

    LOG(LOG_FIRE_LOGIC_EXT, "    + Firing to (%f , %f)", hitPoint.x, hitPoint.y);

    // TODO(gpascualg): This might not be the best place?
    auto qt = entity->cell()->quadtree();
    std::list<MapAwareEntity*> entities;
    qt->retrieve(entities, box.asRect());

    LOG(LOG_FIRE_LOGIC_EXT, "      + Number of candidates %" PRIuPTR, entities.size());

    for (auto*& candidate : entities)
    {
        if (candidate == entity)
        {
            continue;
        }

        if (SAT::get()->collides(candidate->boundingBox(), &box))
        {
            LOG(LOG_FIRE_LOGIC, "Hit %" PRId64 " at (%f, %f)", candidate->id(), candidate->motionMaster()->position().x, candidate->motionMaster()->position().z);

            // TODO(gpascualg): This should aggregate number of hits per target, and set correct id
            Packet* broadcast = Packet::create((uint16_t)PacketOpcodes::FIRE_HIT);
            *broadcast << candidate->id() << static_cast<uint8_t>(WeaponType::MORTAR) << 50.0f;
            Server::map()->broadcastToSiblings(client->entity()->cell(), broadcast);

            // TODO(gpascualg): Dynamic damage based on dist/weapon/etc
            // Apply damage
            static_cast<Entity*>(candidate)->damage(50.0f);
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

    // Real bounding box sizes
    client->entity()->setupBoundingBox({ {-4.28, -16}, {-4.28, 14.77}, {4.28, 15.77}, {4.28, -16} });

    // Set client in world
    map()->addTo(client->entity(), nullptr);
    client->status(Client::Status::IN_WORLD);

    // TODO(gpascualg): Fetch from DB
    // TODO(gpascualg): Move out of here
    for (int i = 0; i < 1; ++i)
    {
        // TODO(gpascualg): Move this out to somewhere else
        static std::default_random_engine randomEngine;
        static std::uniform_real_distribution<> forwardDist(-1, 1); // rage 0 - 1

        MapAwareEntity* entity = newMapAwareEntity(AtomicAutoIncrement<0>::get(), nullptr);
        entity->setupBoundingBox({ {-4.28, -16}, {-4.28, 14.77}, {4.28, 15.77}, {4.28, -16} });
        entity->motionMaster()->teleport({ 0,0,0 });
        entity->motionMaster()->forward(glm::normalize(glm::vec3{ forwardDist(randomEngine), 0, forwardDist(randomEngine) }));
        entity->motionMaster()->generator(new RandomMovement());
        map()->addTo(entity, nullptr);
    }

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

            // Check if packet is valid
            if (client->inMap())
            {
                // Read packet
                uint16_t opcode = client->packet()->read<uint16_t>();
                uint16_t len = client->packet()->read<uint16_t>();

                auto handler = _handlers.find((PacketOpcodes)opcode);
                if (handler == _handlers.end())
                {
                    client->close();
                }
                else
                {
                    auto& opcodeHandler = handler->second;

                    switch (opcodeHandler.cond)
                    {
                        case Condition::NONE:
                            break;

                        case Condition::ALIVE:
                            if (!client->entity() || !static_cast<Entity*>(client->entity())->isAlive())
                            {
                                client->close();
                            }
                            break;
                    }

                    if (client->status() != Client::Status::CLOSED && opcodeHandler.type != HandlerType::NO_CALLBACK)
                    {
                        // Copy packet and craete work-task
                        auto packet = Packet::copy(client->packet(), client->packet()->size());
                        ClientWork* work = new ClientWork(opcodeHandler.work, client, packet);

                        if (opcodeHandler.type == HandlerType::ASYNC_CLIENT)
                        {
                            client->entity()->schedule(work);
                        }
                        else if (opcodeHandler.type == HandlerType::SYNC_SERVER)
                        {
                            this->schedule(work);
                        }
                    }
                }
            }
            else
            {
                LOG(LOG_FATAL, "Should not happen to have a broadcast while not in map");
            }
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
