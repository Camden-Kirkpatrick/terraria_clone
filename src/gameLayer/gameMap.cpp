#include "gameMap.hpp"
#include "asserts.hpp"

void GameMap::create(int w, int h)
{
	*this = {}; // Reset all the map data
	mapBlocks.resize(w * h);
	mapWallBlocks.resize(w * h);

	this->w = w;
	this->h = h;

	// Reset all the block data (all blocks are air)
	for (Block& e : mapBlocks)
	{
		e = {};
	}
	for (Block& e : mapWallBlocks)
	{
		e = {};
	}
}

Block& GameMap::getBlockUnsafe(int x, int y)
{
	permaAssertCommentDevelopement(
		mapBlocks.size() == w * h,
		"Map data not initialized"
	);

	permaAssertCommentDevelopement(
		x >= 0 && y >= 0 && x < w && y < h,
		"getBlockUnsafe out of bounds error"
	);

	return mapBlocks[w * y + x];
}

Block* GameMap::getBlockSafe(int x, int y)
{
	permaAssertCommentDevelopement(
		mapBlocks.size() == w * h,
		"Map data not initialized"
	);

	if (x < 0 || y < 0 || x >= w || y >= h)
		return nullptr;

	return &mapBlocks[w * y + x];
}

Block& GameMap::getWallBlockUnsafe(int x, int y)
{
	permaAssertCommentDevelopement(
		mapWallBlocks.size() == w * h,
		"Map data not initialized"
	);

	permaAssertCommentDevelopement(
		x >= 0 && y >= 0 && x < w && y < h,
		"getBlockUnsafe out of bounds error"
	);

	return mapWallBlocks[w * y + x];
}

Block* GameMap::getWallBlockSafe(int x, int y)
{
	permaAssertCommentDevelopement(
		mapWallBlocks.size() == w * h,
		"Map data not initialized"
	);

	if (x < 0 || y < 0 || x >= w || y >= h)
		return nullptr;

	return &mapWallBlocks[w * y + x];
}