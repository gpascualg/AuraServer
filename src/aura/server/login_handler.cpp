#include "server/aura_server.hpp"
#include "entity/aura_entity.hpp"
#include "database/database.hpp"
#include "debug/debug.hpp"
#include "framework/framework.hpp"
#include "io/packet.hpp"
#include "map/map.hpp"
#include "movement/motion_master.hpp"
#include "movement/movement_generator.hpp"


void AuraServer::handleAccept(Client* client, const boost::system::error_code& error)
{
    LOG(LOG_CLIENT_LIFECYCLE, "Client setup");

    // Setup entity
    _clients.emplace(client->id(), client);

    // TODO(gpascualg): Fetch position from DB
    auto motionMaster = client->entity()->motionMaster();
    motionMaster->teleport({ client->id(), 0, 0 });
    //LOG(LOG_SPAWNS, "Entity spawning at %.2f %.2f", motionMaster->position().x, 0);

    // Real bounding box sizes
    client->entity()->setupBoundingBox({ {-4.28, -16}, {-4.28, 14.77}, {4.28, 15.77}, {4.28, -16} });

    // Set client in world
    map()->addTo(client->entity(), nullptr);
    client->status(Client::Status::IN_WORLD);

    // TODO(gpascualg): Fetch from DB
    // TODO(gpascualg): Move out of here
    // for (int i = 0; i < 1; ++i)
    // {
    //     // TODO(gpascualg): Move this out to somewhere else
    //     static std::default_random_engine randomEngine;
    //     static std::uniform_real_distribution<> forwardDist(-1, 1); // rage 0 - 1

    //     MapAwareEntity* entity = newMapAwareEntity(AtomicAutoIncrement<0>::get(), nullptr);
    //     entity->setupBoundingBox({ {-4.28, -16}, {-4.28, 14.77}, {4.28, 15.77}, {4.28, -16} });
    //     entity->motionMaster()->teleport({ 0,0,0 });
    //     entity->motionMaster()->forward(glm::normalize(glm::vec3{ forwardDist(randomEngine), 0, forwardDist(randomEngine) }));
    //     entity->motionMaster()->generator(new RandomMovement());
    //     map()->addTo(entity, nullptr);
    // }

    // Send ID
    Packet* packet = Packet::create((uint16_t)PacketOpcodes::SET_ID);
    *packet << client->id();
    client->send(packet);

    // Start receiving!
    client->scheduleRead(2);
    Server::handleAccept(client, error);
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
