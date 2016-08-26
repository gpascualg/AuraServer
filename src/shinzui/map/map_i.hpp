#include "map.hpp"
#include "cell.hpp"
#include "debug.hpp"


template <typename E>
Map<E>::Map(int32_t x, int32_t y, uint32_t dx, uint32_t dy) :
    _x(x), _y(y),
    _dx(dx), _dy(dy)
{}

template <typename E>
Cell<E>* Map<E>::addTo2D(int16_t x, int16_t y, E e)
{
    return addTo(offsetOf(x, y), e);
}

template <typename E>
Cell<E>* Map<E>::addTo(int16_t q, int16_t r, E e)
{
    return addTo(Offset(q, r), e);
}

template <typename E>
Cell<E>* Map<E>::addTo(const Offset&& offset, E e)
{
    auto cell = getOrCreate(std::move(offset), true);
    // cell->add(e);
    return cell;
}

template <typename E>
Cell<E>* Map<E>::getOrCreate(int16_t q, int16_t r, bool siblings, uint8_t* createCount)
{
    return getOrCreate(Offset(q, r), siblings, createCount);
}

template <typename E>
Cell<E>* Map<E>::getOrCreate(const Offset&& offset, bool siblings, uint8_t* createCount)
{
    auto it = _cells.find(offset.hash());

    if (it == _cells.end())
    {
        auto result = _cells.emplace(offset.hash(), new Cell<E>(std::move(offset)));
        it = result.first;

        if (createCount)
        {
            *createCount = *createCount + 1;
        }
    }

    if (siblings)
    {
        LOG(LOG_CELLS, "Creating siblings");
        setSiblings((*it).second);
    }

    return (*it).second;
}

enum Siblings
{
    ZERO_MINUS  = 0,
    PLUS_MINUS  = 1,
    PLUS_ZERO   = 2,
    ZERO_PLUS   = 3,
    MINUS_PLUS  = 4,
    MINUS_ZERO  = 5
};

struct SiblingGetter
{
    Siblings idx;
    Siblings reciprocal;
    int8_t dx;
    int8_t dy;
};

SiblingGetter siblingGetters[6] = {
    {ZERO_MINUS,    ZERO_PLUS,  +0, -1},
    {PLUS_MINUS,    MINUS_PLUS, +1, -1},
    {PLUS_ZERO,     MINUS_ZERO, +1, +0},
    {ZERO_PLUS,     ZERO_MINUS, +0, +1},
    {MINUS_PLUS,    PLUS_MINUS, -1, +1},
    {MINUS_ZERO,    PLUS_ZERO,  -1, +0}
};

template <typename E>
void Map<E>::setSiblings(Cell<E>* cell)
{
    if (cell->_siblingCount < 6)
    {
        const Offset& offset = cell->offset();
        int16_t q = offset.q();
        int16_t r = offset.r();

        uint8_t count = 0;

        for (auto& getter : siblingGetters)
        {
            auto sibling = getOrCreate(q + getter.dx, r + getter.dy, false, &count);

            cell->_siblings[getter.idx] = sibling;
            sibling->_siblings[getter.reciprocal] = cell;

            // FIXME: This might not be true :/ Only + 1 if (sibling->_siblings[getter.reciprocal]) was nullptr
            sibling->_siblingCount = sibling->_siblingCount >= 6 ? 6 : sibling->_siblingCount + 1;
        }

        cell->_siblingCount = count;
        if (count == 6)
        {
            cell->addUnconnected();
        }
        else
        {
            // Add cell as connected, search in siblings
            cell->addConnected();

            // Add siblings too
            for (auto& sibling : cell->_siblings)
            {
                sibling->addConnected(cell);
            }
        }
    }
}
