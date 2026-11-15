#pragma once

struct EntityAnimation
{
	int posX = 0; // Which animation frame are we on? (Column)
	int posY = 0; // Which animation is this? (Row)

	float timer = 0.0f;

	void update(float deltaTime, float frameDuration, int frameCount)
	{
		timer -= deltaTime;

		if (timer <= 0)
		{
			timer += frameDuration;
			posX++;
		}

		// Modulo loops back to the first frame of the animation
		posX %= frameCount;
	}

	// If we are using animation 1, and are on the 3rd frame,
	// then switch to animation 2, we dont want to start on the
	// 3rd frame of animaiton 2, we want to start at the beginning
	void setAnimation(int animation)
	{
		// New animation
		if (posY != animation)
		{
			// Start at the beginning of this animation
			posX = 0;
			// Change animation
			posY = animation;
		}
	}
};