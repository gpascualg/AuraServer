#include "map/cell.hpp"
#include "map/map-cluster/cluster.hpp"
#include "map/map.hpp"
#include "map/map_aware_entity.hpp"

#include <stdio.h>
#include <chrono>
#include <list>


class Entity : public MapAwareEntity
{
public:
    using MapAwareEntity::MapAwareEntity;

    Packet* spawnPacket() { return nullptr; };
    Packet* despawnPacket() { return nullptr; };
};

int main()
{
    typedef std::chrono::high_resolution_clock Time;
    typedef std::chrono::nanoseconds ns;

    Map map;

    map.addTo3D({ 0, 0, 0 }, new Entity(0), nullptr);
    for (int k = 1; k <= 1000; k *= 2)
    {
        for (float x = (float)(-50 * k); x < (float)(50 * k); x += rand() / (float)RAND_MAX * 10)
        {
            for (float y = (float)(-5 * k); y < (float)(5 * k); y += rand() / (float)RAND_MAX * 10)
            {
                map.addTo3D({ x, 0, y }, new Entity(k), nullptr);
            }
        }

        map.runScheduledOperations();

        {
            auto t0 = Time::now();
            map.cluster()->update(0);
            auto t1 = Time::now();
        }
    }

    for (int i = 0; i < 10; ++i)
    {
        {
            auto t0 = Time::now();
            map.cluster()->update(0);
            auto t1 = Time::now();
        }
    }

    //LOG_ALWAYS("UpdateCount == CellCount, %d == %d", updateCount, cellCount);

    getchar();

    return 0;
}
