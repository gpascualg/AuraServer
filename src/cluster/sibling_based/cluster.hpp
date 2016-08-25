#pragma once

#include <vector>
#include <array>




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

    static void recurseElements(E e, uint8_t fetchCurrent)
    {
        if (e && e->_fetchLast == fetchCurrent)
        {
            ++e->_fetchLast;
            e->update();

            if (e->_siblingCount)
            {
                for (auto& sibling : e->_siblings)
                {
                    recurseElements(sibling, fetchCurrent);
                }
            }
        }
    }

private:
    Cluster();

    struct Data
    {
        std::vector<E> elements;
    };

private:
    static Cluster<E>* _instance;

    uint32_t _numClusters = 0;
    uint32_t _fetchCurrent = 0;
    std::array<Data*, MaxClusters> _clusters;
};


#include "cluster_i.hpp"
