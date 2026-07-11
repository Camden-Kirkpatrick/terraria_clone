#pragma once
#include <raylib.h>

struct Transform2D
{
	Vector2 pos = {}; // center
	float w = 0;
	float h = 0;

	Vector2 getCenter()			const { return { pos.x, pos.y }; }
	Vector2 getTop()			const { return { pos.x, pos.y - h * 0.5f }; }
	Vector2 getBottom()			const { return { pos.x, pos.y + h * 0.5f}; }
	Vector2 getLeft()			const { return { pos.x - w * 0.5f, pos.y }; }
	Vector2 getRight()			const { return { pos.x + w * 0.5f, pos.y }; }
	Vector2 getTopLeft()		const { return { pos.x - w * 0.5f, pos.y - h * 0.5f }; }
	Vector2 getTopRight()		const { return { pos.x + w * 0.5f, pos.y - h * 0.5f }; }
	Vector2 getBottomLeft()		const { return { pos.x - w * 0.5f, pos.y + h * 0.5f }; }
	Vector2 getBottomRight()	const { return { pos.x + w * 0.5f, pos.y + h * 0.5f }; }

	// Get the top-left corner of the entity
	// AABB = Axis Aligned Bounding Box (Outline box around the entity)
	Rectangle getAABB()
	{
		return { pos.x - w * 0.5f, pos.y - h * 0.5f, w, h };
	}

	bool intersectPoint(Vector2 point, float delta = 0)
	{
		Rectangle aabb = getAABB();
		aabb.x -= delta;
		aabb.y -= delta;
		aabb.width += 2 * delta;
		aabb.height += 2 * delta;

		return CheckCollisionPointRec(point, aabb);
	}

	bool intersectTransform(Transform2D other, float delta = 0)
	{
		Rectangle a = getAABB();
		Rectangle b = other.getAABB();

		a.x -= delta;
		a.y -= delta;
		a.width += 2 * delta;
		a.height += 2 * delta;

		b.x -= delta;
		b.y -= delta;
		b.width += 2 * delta;
		b.height += 2 * delta;

		return CheckCollisionRecs(a, b);
	}
};