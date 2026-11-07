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

	PhysicalEntity physics;

	Vector2& getPosition()
	{
		return physics.transform.pos;
	}

	void render(AssetManager& assetManager);
	void update(float deltaTime);
};