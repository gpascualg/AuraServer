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
