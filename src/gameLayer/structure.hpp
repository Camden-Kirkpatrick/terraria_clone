#pragma once
#include <vector>
#include <raylib.h>
#include "blocks.hpp"
#include "gameMap.hpp"

struct Structure
{
	int w = 0;
	int h = 0;

	std::vector<Block> structureBlocks;
	std::vector<Block> structureWallBlocks;

	void create(int w, int h);

	Block& getBlockUnsafe(int x, int y);
	Block* getBlockSafe(int x, int y);
	Block& getWallBlockUnsafe(int x, int y);
	Block* getWallBlockSafe(int x, int y);

	void copyFromMap(GameMap &map, Vector2 start, Vector2 end);
	void pasteIntoMap(GameMap& map, Vector2 start);

	std::uint16_t getBlockType(int x, int y);
};