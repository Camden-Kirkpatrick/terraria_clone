#include "randomStuff.hpp"

float getRandomFloat(std::ranlux24_base& rng, float min, float max)
{
	std::uniform_real_distribution<float> dist(min, max);
	return dist(rng);
}

float getRandomInt(std::ranlux24_base& rng, int min, int max)
{
	std::uniform_int_distribution<int> dist(min, max);
	return dist(rng);
}

// This function takes a chance value between 0 and 1, and returns true with that probability.
// For example, getRandomChance(rng, 0.25) returns true 25% of the time and false 75% of the time.
bool getRandomChance(std::ranlux24_base& rng, float chance)
{
	float dice = getRandomFloat(rng, 0.0f, 1.0f);
	return dice <= chance;
}