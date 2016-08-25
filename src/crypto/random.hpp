#pragma once

#include <inttypes.h>
#include <array>

template <uint32_t NumTables = 0x3D, uint32_t LoopsPerTable = 0x200>
class Random
{
public:
	Random(uint32_t state0, uint32_t state1);

private:
	void generate();
	void algorithm();

private:
	uint16_t _counter1;
	uint16_t _counter2;
	std::array<uint32_t, NumTables * LoopsPerTable * 2> _table;

	uint32_t _state0;
	uint32_t _state1;
};
