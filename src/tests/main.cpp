#include "cell.hpp"
#include "cluster.hpp"
#include "map.hpp"

#include <stdio.h>

class Entity
{};

int main()
{
    Map<Entity*> map(0, 0, 200, 50);

    LOG_ALWAYS("TESTING CLUSTER MERGE");
    map.addTo2D(20, 20, new Entity());
    map.addTo2D(50, 50, new Entity());
    map.addTo2D(20, 30, new Entity());
    map.addTo2D(30, 30, new Entity());
    map.addTo2D(40, 30, new Entity());
    map.addTo2D(40, 40, new Entity());

    LOG_ALWAYS("CREATING");
    for (float x = 0; x < 100000; x += rand() / (float)RAND_MAX * 10)
    {
        for (float y = 0; y < 10000; y += rand() / (float)RAND_MAX * 10)
        {
            map.addTo2D((int16_t)x, (int16_t)y, new Entity());
        }
    }

    LOG_ALWAYS("UPDATING...");
    Cluster<Cell<Entity*>*>::get()->update();

    LOG_ALWAYS("UPDATING...");
    Cluster<Cell<Entity*>*>::get()->update();

    LOG_ALWAYS("UPDATING...");
    Cluster<Cell<Entity*>*>::get()->update();

    LOG(LOG_DEBUG, "UpdateCount == CellCount, %d == %d", updateCount, cellCount);

    LOG_ALWAYS("DONE (%d cells)\n", map.size());
    getchar();

    return 0;
}
