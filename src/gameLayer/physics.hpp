#pragma once
#include <raylib.h>
#include <raymath.h>



// Vector2 operator overloads
inline Vector2 operator+(const Vector2& a, const Vector2& b)
{
	return { a.x + b.x, a.y + b.y };
}

inline Vector2 operator-(const Vector2& a, const Vector2& b)
{
	return { a.x - b.x, a.y - b.y };
}

inline Vector2 operator*(const Vector2& a, float scalar)
{
	return { a.x * scalar, a.y * scalar };
}

inline Vector2 operator/(const Vector2& a, float scalar)
{
	return { a.x / scalar, a.y / scalar };
}

inline Vector2& operator*=(Vector2& a, float scalar)
{
	a.x *= scalar;
	a.y *= scalar;
	return a;
}

inline Vector2& operator/=(Vector2& a, float scalar)
{
	a.x /= scalar;
	a.y /= scalar;
	return a;
}

inline Vector2& operator+=(Vector2& a, float scalar)
{
	a.x += scalar;
	a.y += scalar;
	return a;
}

inline Vector2& operator-=(Vector2& a, float scalar)
{
	a.x -= scalar;
	a.y -= scalar;
	return a;
}


inline bool operator==(const Vector2& a, const Vector2& b)
{
	return a.x == b.x && a.y == b.y;
}

inline bool operator!=(const Vector2& a, const Vector2& b)
{
	return !(a == b);
}

inline Vector2& operator+=(Vector2& a, const Vector2& b)
{
	a.x += b.x;
	a.y += b.y;
	return a;
}

inline Vector2& operator-=(Vector2& a, const Vector2& b)
{
	a.x -= b.x;
	a.y -= b.y;
	return a;
}

inline Vector2& operator*=(Vector2& a, const Vector2& b)
{
	a.x *= b.x;
	a.y *= b.y;
	return a;
}

inline Vector2& operator/=(Vector2& a, const Vector2& b)
{
	a.x /= b.x;
	a.y /= b.y;
	return a;
}


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

struct PhysicalEntity
{
	Transform2D transform; // position and size
	Vector2 lastPosition = {}; // movement direction = new position - old position

	Vector2 velocity = {};
	Vector2 acceleration = {};

	void teleport(Vector2 pos)
	{
		transform.pos = pos;
		lastPosition = pos;
	}

	void updateForces(float deltaTime)
	{
		velocity += acceleration * deltaTime;
		transform.pos += velocity * deltaTime;

		Vector2 dragVector = Vector2{ velocity.x * std::abs(velocity.x),
									  velocity.y * std::abs(velocity.y) };
		float drag = 0.01f;

		if (Vector2Length(dragVector) * drag * deltaTime > Vector2Length(velocity))
			velocity = {};
		else
			velocity -= dragVector * drag * deltaTime;

		if (Vector2Length(velocity) < 0.01f)
			velocity = {};

		acceleration = {};
	}

	// Called at the end of the frame
	void updateFinal()
	{
		lastPosition = { transform.pos.x, transform.pos.y };
	}
};