#pragma once
#include <vector>
#include "blocks.hpp"

struct Structure
{
	int w = 0;
	int h = 0;

	std::vector<Block> structureBlocks;

	void create(int w, int h);

	Block& getBlockUnsafe(int x, int y);
	Block* getBlockSafe(int x, int y);
};