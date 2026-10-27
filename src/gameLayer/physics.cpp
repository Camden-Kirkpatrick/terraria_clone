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
	// +1/-1 accounts for 1 extra block for rare cases where we get something like 9.9999999, and not 10
	int minX = floor(pos.x - dimensions.x / 2.0f - 1);
	int maxX = ceil(pos.x + dimensions.x / 2.0f + 1);
	int minY = floor(pos.y - dimensions.y / 2.0f - 1);
	int maxY = ceil(pos.y + dimensions.y / 2.0f + 1);

	// Ensure that we dont check for collisions outside the map (player is near the edges of the map)
	minX = std::max(0, minX);
	minY = std::max(0, minY);
	maxX = std::min(mapData.w, maxX);
	maxY = std::min(mapData.h, maxY);

	// Check for a collision with all blocks inside the bounding box
	for (int y = minY; y < maxY; y++)
	{
		for (int x = minX; x < maxX; x++)
		{
			// If we can't collide with this type of block, don't check for a collision
			if (!mapData.getBlockUnsafe(x, y).isCollidable())
				continue;

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



void PhysicalEntity::resolveConstraints(GameMap& mapData)
{
	upTouch = false;
	downTouch = false;
	leftTouch = false;
	rightTouch = false;

	Vector2& pos = transform.pos;

	// Distance moved since last frame
	float distance = Vector2Distance(lastPosition, pos);

	if (distance == 0)
	{
		// No movement happened
		return;
	}

	float GRANULARITY = 0.8f;

	// Normal behavior
	if (distance <= GRANULARITY)
	{
		checkCollisionOnce(pos, mapData);
	}
	// The player moved too much this frame (high delta time value)
	else
	{
		Vector2 newPos = lastPosition;
		Vector2 delta = pos - lastPosition;
		delta = Vector2Normalize(delta);
		delta *= GRANULARITY;

		do
		{
			newPos += delta;
			Vector2 posTest = newPos;
			checkCollisionOnce(newPos, mapData);

			// If this is true, that means that newPos changed because there was a collision
			if (newPos != posTest)
			{
				pos = newPos;
				goto end;
			}
		} while (Vector2Length(pos - newPos) > GRANULARITY);

		// Check for a collision one last time
		checkCollisionOnce(pos, mapData);
	}

end:
	// Ensure the player stays in-bound of the world
	if (pos.x - transform.w / 2 < 0) // The player can't go left of the left world edge
		pos.x = transform.w / 2;
	if (pos.x + transform.w / 2 > mapData.w) // The player can't go right of the right world edge
		pos.x = mapData.w - transform.w / 2;
	if (pos.y + transform.h / 2 > mapData.h) // The player can't fall below the bottom world edge
		pos.y = mapData.h - transform.h / 2;

	// If the player is touching a block and was moving, set the correct speed to 0,
	// so the player is not trying to ram into the block continuously
	if (leftTouch && velocity.x < 0)
		velocity.x = 0;
	if (rightTouch && velocity.x > 0)
		velocity.x = 0;
	if (upTouch && velocity.y < 0)
		velocity.y = 0;
	if (downTouch && velocity.y > 0)
		velocity.y = 0;
}