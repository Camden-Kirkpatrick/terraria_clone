#pragma once
#include <vector>
#include "blocks.hpp"

struct GameMap
{
	int w = 0;
	int h = 0;

	// Vectors to store the block data for the main blocks and wall blocks.
	// They are accessed using 2D coordinates (x, y) but stored in a 1D vector for cache efficiency.
	std::vector<Block> mapBlocks;
	std::vector<Block> mapWallBlocks;

	void create(int w, int h);

	// Unsafe versions of the getBlock functions will assert and crash if the coordinates are out of bounds.
	Block& getBlockUnsafe(int x, int y);
	// Safe versions of the getBlock functions will return nullptr if the coordinates are out of bounds.
	Block* getBlockSafe(int x, int y);
	Block& GameMap::getWallBlockUnsafe(int x, int y);
	Block* GameMap::getWallBlockSafe(int x, int y);

	// Helper function to get the block type at (x, y) safely, returning air for out-of-bounds coordinates.
	// This is used for checking the blocks around woodLogs, to see if the texture needs to be updated.
	std::uint16_t getBlockType(int x, int y);
};