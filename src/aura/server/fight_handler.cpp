#include "server/aura_server.hpp"
#include "entity/aura_entity.hpp"
#include "database/database.hpp"
#include "debug/debug.hpp"
#include "framework/framework.hpp"
#include "io/packet.hpp"
#include "map/cell.hpp"
#include "map/map.hpp"
#include "map/map-cluster/cluster.hpp"
#include "map/map_aware_entity.hpp"
#include "map/quadtree.hpp"
#include "movement/motion_master.hpp"
#include "movement/movement_generator.hpp"


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
