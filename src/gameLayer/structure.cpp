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