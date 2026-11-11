#pragma once

#include "../physics.hpp"
#include <raylib.h>
#include <random>

struct AssetManager;

struct Slime
{
	Slime()
	{
		physics.transform.w = 0.8f;
		physics.transform.h = 0.8f;
	}

	enum
	{
		STATE_WANDERING = 0,
		STATE_CHASING,
	};

	int currentState = STATE_WANDERING;
	float changeStateTimer = 1.0f;

	float jumpTimer = 5.0f;
	float moveSpeed = 0.0f; // positive = right-ward movement

	PhysicalEntity physics;

	Vector2& getPosition()
	{
		return physics.transform.pos;
	}

	void render(AssetManager& assetManager);
	void update(float deltaTime, std::ranlux24_base &rng, Vector2 playerPosition);
};