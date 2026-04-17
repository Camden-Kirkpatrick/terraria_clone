#include "helpers.hpp"

// Returns the source rectangle for a tile at grid position (x, y) in the texture atlas.
Rectangle getTextureAtlas(int x, int y, int cellSizePixelsX, int cellSizePixelsY)
{
	return Rectangle{
		(float)x * cellSizePixelsX,
		(float)y * cellSizePixelsY,
		(float)cellSizePixelsX,
		(float)cellSizePixelsY
	};
}