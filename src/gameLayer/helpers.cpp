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

Rectangle getRectangleForEntity(Transform2D transform, float textureW, float textureH)
{
	Transform2D result = transform;
	result.w = textureW;
	result.h = textureH;

	// Move the sprite so that the bottom of the sprite matches the bottom of the collider
	result.pos.y -= (result.h - transform.h) / 2;

	return result.getAABB();
}