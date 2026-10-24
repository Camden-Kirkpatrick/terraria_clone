#include "physics.hpp"
#include "gameMap.hpp"
#include <raymath.h>

Vector2 PhysicalEntity::performCollisionOnOneAxis(GameMap& mapData, Vector2 pos, Vector2 delta)
{
	// No change in position from the last frame
	if (delta.x == 0 && delta.y == 0)
		return pos;

	Vector2 dimensions = { transform.w, transform.h };

	// A bounding box with the blocks the player might be colliding with
	int minX = floor(pos.x - dimensions.x / 2.0f - 1);
	int maxX = ceil(pos.x + dimensions.x / 2.0f + 1);
	int minY = floor(pos.y - dimensions.y / 2.0f - 1);
	int maxY = ceil(pos.y + dimensions.y / 2.0f + 1);

	// Ensure that we dont check for collisions outside the map (player is near the edges of the map)
	minX = std::max(0, minX);
	minY = std::max(0, minY);
	maxX = std::min(mapData.w, maxX);
	maxY = std::min(mapData.h, maxY);

	for (int y = minY; y < maxY; y++)
	{
		for (int x = minX; x < maxX; x++)
		{
			// Can we collide with this type of block?
			if (mapData.getBlockUnsafe(x, y).isCollidable())
			{
				Transform2D entity;
				entity.pos = pos;
				entity.w = dimensions.x;
				entity.h = dimensions.y;

				Transform2D block;
				block.pos = { x + 0.5f, y + 0.5f };
				block.w = 1;
				block.h = 1;

				// Is there a collision?
				if (entity.intersectTransform(block, -0.00005f))
				{
					// Did we move horizontally?
					if (delta.x != 0)
					{
						// Did we move left?
						if (delta.x < 0)
						{
							leftTouch = 1;
							// x is the x value in the top-left corner of the block
							// add 1 to get to the right side
							// add half the player width to put it one block to the right
							pos.x = x + 1.0f + dimensions.x / 2;
							return pos;
						}
						else // We moved right
						{
							rightTouch = 1;
							// We are on the left side of the block, and we are pushing
							// the player left, so we dont need to subtract 1 
							pos.x = x - dimensions.x / 2;
							return pos;
						}
					}
					// Did we move vertically?
					else if (delta.y != 0)
					{
						// Did we move up?
						if (delta.y < 0)
						{
							upTouch = 1;
							pos.y = y + 1.0f + dimensions.y / 2;
							return pos;
						}
						else // We moved down
						{
							downTouch = 1;
							pos.y = y - dimensions.y / 2;
							return pos;
						}
					}
				}
			}
		}
	}
	return pos;
}



void PhysicalEntity::checkCollisionOnce(Vector2& pos, GameMap& mapData)
{
	Vector2 delta = pos - lastPosition;

	// Move the player on the x-axis
	Vector2 newPos = performCollisionOnOneAxis(mapData, { pos.x, lastPosition.y }, { delta.x, 0 });

	// MOve the player on the y-axis
	pos = performCollisionOnOneAxis(mapData, { newPos.x, pos.y }, { 0, delta.y });
}

