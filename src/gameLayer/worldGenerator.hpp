#pragma once
#include "gameMap.hpp"

#define DEFAULT_SEED 1234

struct WorldNoise
{
	int dirtMountainOctaves;
	float dirtMountainFrequency;
	int stoneMountainOctaves;
	float stoneMountainFrequency;

	int dirtPlainOctaves;
	float dirtPlainFrequency;
	int stonePlainOctaves;
	float stonePlainFrequency;

	int biomeOctaves;
	float biomeFrequency;
};

extern WorldNoise worldNoise;
extern int seed;

void resetWorldNoise();
float lerp(float a, float b, float t);
void generateWorld(GameMap& gameMap, const int WIDTH, const int HEIGHT, int seed = DEFAULT_SEED, bool resetNoise = true);