#pragma once

#include <array>
#include <map>
#include <vector>


// Could we generalize this? :(
constexpr uint32_t MaxClusters = 12;

template <typename E>
class Cluster
{
    template <typename E2, uint32_t NSiblings>
    friend class ClusterElement;

    /*
    static_assert(
        std::is_base_of<ClusterElement, E>::value,
        "E must be a descendant of ClusterElement"
    );
   */

public:
    static Cluster<E>* get()
    {
        if (!_instance)
        {
            _instance = new Cluster();
        }
        return _instance;
    }

    void update();

private:
    Cluster();

private:
    static Cluster<E>* _instance;

    uint32_t _numClusters = 0;
    uint32_t _fetchCurrent = 0;

    std::vector<std::vector<E>*> _clusters;
};


#include "cluster_i.hpp"
