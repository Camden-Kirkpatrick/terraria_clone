#pragma once
#include <cstdint>

struct Block
{
	enum
	{
		air = 0,
		dirt,
		stone,

		BLOCKS_COUNT
	};
	std::uint16_t type = air;
};