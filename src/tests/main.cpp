#include "cell.hpp"
#include "cluster.hpp"
#include "map.hpp"
#include "map_aware_entity.hpp"

#include <stdio.h>
#include <chrono>


class Entity : public MapAwareEntity
{
    uint32_t id() { return 0; }

    void onAdded(Cell* cell) override {}
    void onRemoved(Cell* cell) override {}
    void update(uint64_t elapsed) override {}
};

int main()
{
    typedef std::chrono::high_resolution_clock Time;
    typedef std::chrono::nanoseconds ns;

    Map map;

/*
    LOG_ALWAYS("TESTING CLUSTER MERGE");
    map.addTo2D(20, 20, new Entity());
    map.addTo2D(50, 50, new Entity());
    map.addTo2D(20, 30, new Entity());
    map.addTo2D(30, 30, new Entity());
    map.addTo2D(40, 30, new Entity());
    map.addTo2D(40, 40, new Entity());
*/

    map.addTo2D(0, 0, new Entity());
    for (int k = 1; k <= 1000; k *= 2)
    {
        LOG_ALWAYS("CREATING");
        for (float x = -50 * k; x < 50 * k; x += rand() / (float)RAND_MAX * 10)
        {
            for (float y = -5 * k; y < 5 * k; y += rand() / (float)RAND_MAX * 10)
            {
                map.addTo2D(x, y, new Entity());
            }
        }

        map.runScheduledOperations();

        LOG_ALWAYS("UPDATING %d CELLS", map.size());
        {
            auto t0 = Time::now();
            map.cluster()->update(0);
            auto t1 = Time::now();

            LOG_ALWAYS("\tDONE IN %f",  std::chrono::duration_cast<ns>(t1 - t0).count() / 1000000.0f);
        }
    }

    for (int i = 0; i < 10; ++i)
    {
        LOG_ALWAYS("UPDATING %d CELLS", map.size());
        {
            auto t0 = Time::now();
            map.cluster()->update(0);
            auto t1 = Time::now();

            LOG_ALWAYS("\tDONE IN %f",  std::chrono::duration_cast<ns>(t1 - t0).count() / 1000000.0f);
        }
    }

    //LOG_ALWAYS("UpdateCount == CellCount, %d == %d", updateCount, cellCount);

    getchar();

    return 0;
}
