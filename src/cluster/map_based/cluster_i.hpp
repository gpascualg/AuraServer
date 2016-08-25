#include "cluster.hpp"
#include "common.hpp"
#include "debug.hpp"


template <typename E>
Cluster<E>* Cluster<E>::_instance = nullptr;


template <typename E>
Cluster<E>::Cluster()
{
}

template <typename E>
void Cluster<E>::update()
{
    for (auto& cluster : _clusters)
    {
        LOG(LOG_CLUSTERS, "-] Updating cluster");
        for (auto& el : *cluster)
        {
            if (el->_fetchLast == _fetchCurrent)
            {
                ++el->_fetchLast;
                el->update();
            }
        }
    }

    ++_fetchCurrent;
}
