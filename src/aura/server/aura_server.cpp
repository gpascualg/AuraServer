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
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <thread>

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>


const uint16_t MAX_PACKET_LEN = 1000;
const TimeBase WORLD_HEART_BEAT = TimeBase(50);

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
    }
}

void AuraServer::handleRead(Client* client, const boost::system::error_code& error, size_t size)
{
    IF_LOG(LOG_LEVEL_DEBUG, LOG_PACKET_RECV)
    {
        if (!error)
        {
            std::stringstream ss;

            ss << "Recv bytes at [" << time(NULL) << "] - Size: " << size << "\n";
            for (uint16_t i = 0; i < client->packet()->size(); ++i)
            {
                ss << std::setfill('0') << std::setw(2) << std::hex << (uint8_t)client->packet()->data()[i] << " ";
            }

            LOG_DEBUG(LOG_PACKET_RECV, "%s", ss.str().c_str());
        }
    }

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

// TODO(gpascualg): Move this out to somewhere else
static std::default_random_engine randomEngine;
static std::uniform_real_distribution<> forwardDist(-1, 1); // rage 0 - 1

void AuraServer::onCellCreated(Cell* cell)
{
    auto future = Framework::get()->database()->query<std::vector<Entity*>>("aura", [this, cell](const mongocxx::database& db) {
        bsoncxx::builder::stream::document filter_builder;
        filter_builder << "map.q" << cell->offset().q() << "map.r" << cell->offset().r();

        auto cursor = db["mobs"].find(filter_builder.view());
        std::vector<Entity*> entities;

        for (auto&& doc : cursor)
        {
            Entity* entity = static_cast<Entity*>(newMapAwareEntity(AtomicAutoIncrement<0>::get(), nullptr));
            entity->mongoId(doc["_id"].get_oid());
            entity->setupBoundingBox({ {-4.28, -16}, {-4.28, 14.77}, {4.28, 15.77}, {4.28, -16} });
            entity->motionMaster()->teleport({ doc["map"]["x"].get_double(), doc["map"]["y"].get_double(), doc["map"]["z"].get_double() });
            entity->motionMaster()->forward(glm::normalize(glm::vec3{ forwardDist(randomEngine), 0, forwardDist(randomEngine) }));
            entity->motionMaster()->generator(new RandomMovement());

            entities.emplace_back(entity);
        }

        return entities;
    });

    auto work = new FutureWork<std::vector<Entity*>>(MAKE_HANDLER(onCellLoaded), nullptr, std::move(future));
    schedule(work);
}

AbstractWork* AuraServer::onCellLoaded(FutureWork<std::vector<Entity*>>* work)
{
    std::vector<Entity*> entities = work->get();
    for (auto entity : entities)
    {
        map()->addTo(entity, nullptr);
    }

    return nullptr;
}

void AuraServer::onCellDestroyed(Cell* cell)
{
    for (auto pair : cell->entities())
    {
        auto entity = static_cast<Entity*>(pair.second);
        LOG_ASSERT(!entity->client(), "A cell with a client is being deleted");

        bsoncxx::builder::stream::document filter_builder;
        filter_builder << "_id" << entity->mongoId();

        bsoncxx::builder::stream::document update_builder;
        update_builder << "$set" << open_document << "map" << open_document
            << "q" << cell->offset().q()
            << "r" << cell->offset().r()
            << "x" << entity->motionMaster()->position().x
            << "y" << entity->motionMaster()->position().y
            << "z" << entity->motionMaster()->position().z << close_document << close_document;

        Framework::get()->database()->query<bool>("aura", 
            [filter_doc = bsoncxx::document::value { filter_builder.view() }, update_doc = bsoncxx::document::value { update_builder.view() }](const mongocxx::database& db) {
            db["mobs"].update_one(filter_doc.view(), update_doc.view());
            return true;
        });

        destroyMapAwareEntity(entity);
    }
}

MapAwareEntity* AuraServer::newMapAwareEntity(uint64_t id, Client* client)
{
    return static_cast<MapAwareEntity*>(_entityPool.construct(id, client));
}

void AuraServer::destroyMapAwareEntity(MapAwareEntity* entity)
{
    _entityPool.destroy(static_cast<Entity*>(entity));
}

void AuraServer::iterateClients(std::function<void(Client* client)> callback)
{
    for (auto pair : _clients)
    {
        callback(pair.second);
    }
}
