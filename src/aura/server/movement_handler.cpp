#include "server/aura_server.hpp"
#include "entity/aura_entity.hpp"
#include "database/database.hpp"
#include "debug/debug.hpp"
#include "framework/framework.hpp"
#include "io/packet.hpp"
#include "map/map.hpp"
#include "movement/motion_master.hpp"
#include "movement/movement_generator.hpp"


AbstractWork* AuraServer::handleForwardChange(ClientWork* work)
{
    Client* client = work->executor();
    Packet* packet = work->packet();
    auto* entity = client->entity();

    float rotation = packet->read<float>();
    auto& transform = entity->transform();
    entity->rotate(rotation);

    Packet* broadcast = Packet::create((uint16_t)PacketOpcodes::FORWARD_CHANGE_RESP);
    *broadcast << client->id();
    *broadcast << rotation;

    if (rotation == 0)
    {
        *broadcast << transform.Forward;
    }

    Server::map()->broadcastToSiblings(client->entity()->cell(), broadcast);
    return nullptr;
}

AbstractWork* AuraServer::handleSpeedChange(ClientWork* work)
{
    Client* client = work->executor();
    Packet* packet = work->packet();
    auto* entity = client->entity();

    auto& transform = entity->transform();
    int8_t speed = packet->read<int8_t>();
    entity->move(speed);

    Packet* broadcast = Packet::create((uint16_t)PacketOpcodes::SPEED_CHANGE_RESP);
    *broadcast << client->id() << speed;
    *broadcast << transform.Position;

    Server::map()->broadcastToSiblings(client->entity()->cell(), broadcast);
    return nullptr;
}
