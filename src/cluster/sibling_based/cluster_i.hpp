#include "cluster.hpp"
#include "common.hpp" // TODO: debug.hpp


template <typename E>
Cluster<E>* Cluster<E>::_instance = nullptr;


template <typename E>
Cluster<E>::Cluster()
{
    for (auto*& cluster : _clusters)
    {
        cluster = new Data;
    }
}

template <typename E>
void Cluster<E>::update()
{
    for (auto*& cluster : _clusters)
    {
        LOG(LOG_CLUSTERS, "-] Updating cluster");

        for (auto it = cluster->elements.begin(); it != cluster->elements.end();)
        {
            auto element = *it;
            ++it;

            recurseElements(element, _fetchCurrent);
        }
    }

    ++_fetchCurrent;
}
