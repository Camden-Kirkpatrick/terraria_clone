#pragma once
#include "gameMap.hpp"

struct WorldNoise
{
	int dirtMountainOctaves = 1;
	float dirtMountainFrequency = 0.02f;
	int stoneMountainOctaves = 4;
	float stoneMountainFrequency = 0.01f;

	int dirtPlainOctaves = 1;
	float dirtPlainFrequency = 0.0025f;
	int stonePlainOctaves = 1;
	float stonePlainFrequency = 0.005f;

	int biomeOctaves = 1;
	float biomeFrequency = 0.00025f;
};

extern WorldNoise worldNoise;

void generateWorld(GameMap& gameMap, const int WIDTH = 900, const int HEIGHT = 500, int seed = 2112);