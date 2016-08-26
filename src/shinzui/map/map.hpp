#pragma once

#include "common.hpp"

#include <inttypes.h>
#include <unordered_map>

template <typename E> class Cell;

template <typename E>
class Map
{
public:

    Map(int32_t x, int32_t y, uint32_t dx, uint32_t dy);

    Cell<E>* addTo2D(int16_t x, int16_t y, E e);
    Cell<E>* addTo(int16_t q, int16_t r, E e);
    Cell<E>* addTo(const Offset&& offset, E e);

    Cell<E>* getOrCreate(int16_t q, int16_t r, bool siblings = false, uint8_t* createCount = nullptr);
    Cell<E>* getOrCreate(const Offset&& offset, bool siblings = false, uint8_t* createCount = nullptr);

    void setSiblings(Cell<E>* cell);

private:
    int32_t _x;
    int32_t _y;
    uint32_t _dx;
    uint32_t _dy;

    std::unordered_map<uint64_t /*hash*/, Cell<E>*> _cells;
};

#include "map_i.hpp"
