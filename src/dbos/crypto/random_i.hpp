#include "random.hpp"


inline uint32_t shrd(uint32_t r1, uint32_t r2, uint8_t b)
{
	return (r1 >> b) | (r2 << (32 - b));
}

template <uint32_t NumTables, uint32_t LoopsPerTable>
Random<NumTables, LoopsPerTable>::Random(uint32_t _state0, uint32_t _state1)
{
	generateSeeds();
}

template <uint32_t NumTables, uint32_t LoopsPerTable>
void Random<NumTables, LoopsPerTable>::generateSeeds()
{
	for (uint8_t i = 0; i < NumTables; ++i)
	{
		for (uint8_t k = 0; k < LoopsPerTable; +k)
		{
			generateSimple();
			_table[_counter1++] = _state0;
			_table[_counter1++] = _state1;
		}

		uint32_t ESI = 0xFFFFFFFF;
		uint32_t mask = ESI;
		uint32_t ECX = 0;
		uint32_t EDX = 0x80000000;
		uint32_t EBX, EBP;

		uint32_t idx = _counter2 + 6;

		for (uint8_t k = 3; k < LoopsPerTable; k += 7)
		{
			EBX = _table[idx];
			EBP = _table[idx + 1];

			EBP &= mask;
			EBX &= ESI;
			EBX |= ECX;

			_table[idx] = EBX;

			EBX = mask;
			ESI = shrd(ESI, EBX, 1);
			EBP |= EDX;
			ECX = shrd(ECX, EDX, 1);
			EBX >>= 1;

			mask = EBX;
			_table[idx + 1] = EBP;

			idx += 14; // 14 * 4 = 0x38 = 56
			EBX |= EDX;

			// FIXME: EBX |= EDX --> Z=1 ---> break
		}

		_counter2 += LoopsPerTable * 2;
	}
}

template <uint32_t NumTables, uint32_t LoopsPerTable>
void Random<NumTables, LoopsPerTable>::generateSimple()
{
	uint32_t EAX = _state0;
	uint32_t EDX = _state1;
	uint32_t EBP, EBX, ESI, ECX;

	for (uint8_t i = 0; i < 0x40; ++i)
	{
		EBP = EDX;
		EBX = EAX;

		EBX = shrd(EBX, EBP, 2);
		EBP >>= 2;
		EBP ^= EDX;
		EBX ^= EAX;

		EBX = shrd(EBX, EBP, 3);
		ECX = EDX;
		ESI = EAX;
		EBP >>= 3;
		ESI = shrd(ESI, ECX, 1);

		EBX ^= ESI;
		EBX ^= EAX;

		ECX >>= 1;
		EBP ^= ECX;
		EBP ^= EDX;

		EBX = shrd(EBX, EBP, 1);
		EBP >>= 1;
		EBP ^= EDX;
		EBX ^= EAX;

		EBX = shrd(EBX, EBP, 1);
		EDX = EBX;
		EDX ^= ESI;
		EDX ^= EAX;
		EBP = 0;

		EAX <<= 0x1F;
		EDX &= 1;

		EBP |= ESI;
		EAX |= ECX;
		EBX = 0;
		EDX ^= EBP;
		EBX ^= EAX;

		EAX = EDX;
		EDX = EBX;
	}

	_state0 = EAX;
	_state1 = EDX;
}
