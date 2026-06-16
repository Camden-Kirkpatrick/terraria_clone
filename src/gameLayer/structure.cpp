#include "structure.hpp"
#include "asserts.hpp"

void Structure::create(int w, int h)
{
	*this = {};
	structureBlocks.resize(w * h);
	structureWallBlocks.resize(w * h);

	this->w = w;
	this->h = h;
}

Block& Structure::getBlockUnsafe(int x, int y)
{
	permaAssertCommentDevelopement(
		structureBlocks.size() == w * h,
		"Structure data not initialized"
	);

	permaAssertCommentDevelopement(
		x >= 0 && y >= 0 && x < w && y < h,
		"getBlockUnsafe out of bounds error"
	);

	return structureBlocks[w * y + x];
}

Block* Structure::getBlockSafe(int x, int y)
{
	permaAssertCommentDevelopement(
		structureBlocks.size() == w * h,
		"Structure data not initialized"
	);

	if (x < 0 || y < 0 || x >= w || y >= h)
		return nullptr;

	return &structureBlocks[w * y + x];
}

Block& Structure::getWallBlockUnsafe(int x, int y)
{
	permaAssertCommentDevelopement(
		structureWallBlocks.size() == w * h,
		"Structure data not initialized"
	);

	permaAssertCommentDevelopement(
		x >= 0 && y >= 0 && x < w && y < h,
		"getBlockUnsafe out of bounds error"
	);

	return structureWallBlocks[w * y + x];
}

Block* Structure::getWallBlockSafe(int x, int y)
{
	permaAssertCommentDevelopement(
		structureWallBlocks.size() == w * h,
		"Structure data not initialized"
	);

	if (x < 0 || y < 0 || x >= w || y >= h)
		return nullptr;

	return &structureWallBlocks[w * y + x];
}

// Copy an area from the map and store it in a structure
// start and end define the area being selected
void Structure::copyFromMap(GameMap& map, Vector2 start, Vector2 end)
{
	// Make sure the data passed in is valid
	if (end.x >= map.w)
		end.x = map.w - 1;
	if (end.y >= map.h)
		end.y = map.h - 1;

	if (end.x < 0)
		end.x = 0;
	if (end.y < 0)
		end.y = 0;

	if (start.x < 0)
		start.x = 0;
	if (start.y < 0)
		start.y = 0;

	if (start.x > end.x)
		std::swap(start.x, end.x);
	if (start.y > end.y)
		std::swap(start.y, end.y);

	Vector2 size = Vector2{ end.x - start.x + 1, end.y - start.y + 1 };

	// Allocate memory for the structure
	create(size.x, size.y);

	// Copy the blocks from the map into the structure
	for (int y = 0; y < size.y; y++)
	{
		for (int x = 0; x < size.x; x++)
		{
			getBlockUnsafe(x, y) = map.getBlockUnsafe(x + start.x, y + start.y);
			getWallBlockUnsafe(x, y) = map.getWallBlockUnsafe(x + start.x, y + start.y);
		}
	}
}

// Copy the structure into the map
void Structure::pasteIntoMap(GameMap& map, Vector2 start)
{
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			// Get the map blocks that will be overwritten with the structure
			Block* b = map.getBlockSafe(x + start.x, y + start.y);
			// Place the structure block in the world
			if (b)
				*b = getBlockUnsafe(x, y);

			Block* wb = map.getWallBlockSafe(x + start.x, y + start.y);
			if (b)
				*wb = getWallBlockUnsafe(x, y);
		}
	}
}