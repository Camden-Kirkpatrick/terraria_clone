#include "slime.hpp"
#include "../assetManager.hpp"
#include "../helpers.hpp"
#include "../randomStuff.hpp"

void Slime::render(AssetManager& assetManager)
{
	auto aabb = getRectangleForEntity(physics.transform, 1, 1);

	DrawTexturePro(
		assetManager.slime,
		getTextureAtlas(0, 0, 32, 32),
		aabb,
		{ 0, 0 },
		0.0f,
		WHITE
	);
}

void Slime::update(float deltaTime, std::ranlux24_base &rng, Vector2 playerPosition)
{
	changeStateTimer -= deltaTime;

	if (changeStateTimer <= 0)
	{
		changeStateTimer = getRandomFloat(rng, 1.0f, 5.0f);

		float distanceToPlayer = Vector2Distance(playerPosition, getPosition());

		if (distanceToPlayer <= 20)
		{
			if (getRandomChance(rng, 0.8f))
				currentState = STATE_CHASING;
			else
				currentState = STATE_WANDERING;
		}
		else
			currentState = STATE_WANDERING;
	}

	// The slime only moves when it jumps, not when it's on the ground
	if (physics.downTouch)
		moveSpeed = 0.0f;

	jumpTimer -= deltaTime;


	switch (currentState)
	{
		case STATE_WANDERING:
		{
			if (jumpTimer <= 0)
			{
				jumpTimer = getRandomFloat(rng, 3.0f, 12.0f);

				physics.jump();
				moveSpeed = getRandomFloat(rng, -7.0f, 7.0f);
			}
		}
		break;

		case STATE_CHASING:
		{
			if (jumpTimer <= 0)
			{
				jumpTimer = getRandomFloat(rng, 1.0f, 6.0f);

				physics.jump();

				if (playerPosition.x > getPosition().x)
					moveSpeed = getRandomFloat(rng, 1.0f, 7.0f);
				else
					moveSpeed = -getRandomFloat(rng, 1.0f, 7.0f);
			}
		}
		break;

		default:
			break;
	}

	// Move the slime
	if (moveSpeed != 0.0f)
		getPosition().x += moveSpeed * deltaTime;
}