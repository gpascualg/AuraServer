#pragma once

#include <inttypes.h>
#include <array>

#include "cluster.hpp"


template <typename E> class Cluster;

static int updateCount = 0;

template <typename E, uint32_t NSiblings>
class ClusterElement
{
    friend class Cluster<E>;

public:
    ClusterElement()
    {
        for (auto& e : _siblings)
        {
            e = nullptr;
        }
    }

    void createClusterOrAddSelf();

    virtual void update()
    {
        LOG(LOG_CLUSTERS, "\t-] Updating %X", this);
        ++updateCount;
    }

protected:
    uint8_t _siblingCount = 0;
    std::array<E, NSiblings> _siblings;

private:
    uint8_t _fetchLast = 0;
};


template <typename E, uint32_t NSiblings>
void ClusterElement<E, NSiblings>::createClusterOrAddSelf()
{
    auto* cluster = Cluster<E>::get();
    LOG(LOG_CLUSTERS, "Cluster created at %d", cluster->_numClusters);

    cluster->_clusters[cluster->_numClusters]->elements.emplace_back((E)this);
    cluster->_numClusters = (cluster->_numClusters + 1) % MaxClusters;
}
