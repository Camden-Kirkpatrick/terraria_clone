#pragma once
#include <vector>
#include "blocks.hpp"

struct GameMap
{
	int w = 0;
	int h = 0;

	std::vector<Block> mapData;

	void create(int w, int h);

	Block& getBlockUnsafe(int x, int y);
	Block* getBlockSafe(int x, int y);
};