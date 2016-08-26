#pragma once

#define _USE_MATH_DEFINES

#include <inttypes.h>
#include <math.h>

struct Offset
{
public:
    constexpr Offset(int16_t q, int16_t r):
        _q(q), _r(r)
    {}

    inline constexpr int16_t q() const { return _q; }
    inline constexpr int16_t r() const { return _r; }
    inline constexpr int16_t s() const { return - _q - _r; }

    constexpr uint64_t hash() const
    {
        return ((uint64_t)q() << 32) | ((uint32_t)r() << 16) | ((uint32_t)s() << 0);
    }

private:
    const int16_t _q;
    const int16_t _r;
};

// Windows does not define "cos" as constexpr yet... :(
#ifdef _WIN32
	constexpr uint8_t cellSize = 9;
	const double slopeX = cos(15 * M_PI / 180.0) * cellSize;

	const Offset offsetOf(int32_t x, int32_t y)
	{
		return Offset(x / slopeX, y / cellSize);
	}
#else
	constexpr uint8_t cellSize = 9;
	constexpr double slopeX = cos(15 * M_PI / 180.0) * cellSize;

	constexpr Offset offsetOf(int32_t x, int32_t y)
	{
		return Offset(x / slopeX, y / cellSize);
	}
#endif
