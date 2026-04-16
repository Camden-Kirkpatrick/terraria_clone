#include "gameMap.hpp"
#include "asserts.hpp"

void GameMap::create(int w, int h)
{
	*this = {}; // Reset all the map data
	mapData.resize(w * h);

	this->w = w;
	this->h = h;

	// Reset all the block data (all blocks are air)
	for (Block& e : mapData)
	{
		e = {};
	}
}

Block& GameMap::getBlockUnsafe(int x, int y)
{
	permaAssertCommentDevelopement(
		mapData.size() == w * h,
		"Map data not initialized"
	);

	permaAssertCommentDevelopement(
		x >= 0 && y >= 0 && x < w && y < h,
		"getBlockUnsafe out of bounds error"
	);

	return mapData[w * y + x];
}

Block* GameMap::getBlockSafe(int x, int y)
{
	permaAssertCommentDevelopement(
		mapData.size() == w * h,
		"Map data not initialized"
	);

	if (x < 0 || y < 0 || x >= w || y >= h)
		return nullptr;

	return &mapData[w * y + x];
}