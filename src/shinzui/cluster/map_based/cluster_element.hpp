#pragma once

#include <inttypes.h>
#include <algorithm>
#include <array>

#include "cluster.hpp"
#include "debug.hpp"


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

    void addUnconnected();
    void addConnected(E to = nullptr);

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
    std::vector<E>* _cluster = nullptr;
};


template <typename E, uint32_t NSiblings>
void ClusterElement<E, NSiblings>::addUnconnected()
{
    auto* cluster = Cluster<E>::get();
    LOG(LOG_CLUSTERS, "addUnconnected at %d", cluster->_numClusters);

    // Create cluster if it can be created
    if (cluster->_clusters.size() < MaxClusters)
    {
        cluster->_clusters.emplace_back(new std::vector<E> { });
        this->_cluster = cluster->_clusters.back();
    }
    else
    {
        this->_cluster = *(cluster->_clusters.begin() + (int)(rand() / (float)RAND_MAX * MaxClusters));
    }

    // Add to cluster
    this->_cluster->emplace_back((E)this);

    // Add siblings
    for (auto& sibling : _siblings)
    {
        sibling->addConnected((E)this);
    }

    cluster->_numClusters = (cluster->_numClusters + 1) % MaxClusters;
}

template <typename E, uint32_t NSiblings>
void ClusterElement<E, NSiblings>::addConnected(E to)
{
    // If no "to" supplied, check all siblings
    for (int i = 0; i < NSiblings && !to; ++i)
    {
        if (_siblings[i] && _siblings[i]->_cluster)
        {
            to = _siblings[i];
        }
    }

    // Merge clusters
    if (this->_cluster && this->_cluster != to->_cluster)
    {
        LOG(LOG_CLUSTERS, "SHOULD MERGE CLUSTERS, OLD %u : NEW %u", this->_cluster, to->_cluster);

        // Step 1, move to the other vectorkl
        std::move(this->_cluster->begin(), this->_cluster->end(), std::back_inserter(*to->_cluster));

        // Step 2, remove old map
        auto& vec = Cluster<E>::get()->_clusters;
        vec.erase(std::find(vec.begin(), vec.end(), this->_cluster));

        // Step 3, update all references
        for (auto& el : *this->_cluster)
        {
            el->_cluster = to->_cluster;
        }
    }

    // Add only if not already in
    if (!this->_cluster)
    {
        LOG(LOG_CLUSTERS, "addConnected at %x", to->_cluster);
        to->_cluster->emplace_back((E)this);
        this->_cluster = to->_cluster;
    }
}
