#pragma once
#include "gameMap.hpp"

#define DEFAULT_SEED 1234

struct WorldNoise
{
	int dirtMountainOctaves;
	float dirtMountainFrequency;
	int stoneMountainOctaves;
	float stoneMountainFrequency;

	int minDirtMountainThickness;   // Minimum amount of dirt above stone
	int maxDirtMountainThickness;   // Maximum blocks of dirt above stone
	int minStoneMountainStart;      // Stone layer is at least 80 blocks from the top
	int maxStoneMountainStart;      // The top of the stone layer is at most 150 blocks from the top

	int dirtPlainOctaves;
	float dirtPlainFrequency;
	int stonePlainOctaves;
	float stonePlainFrequency;

	int biomeOctaves;
	float biomeFrequency;

	int caveOctaves;
	float caveFrequency;

	// When the cave noise is in this range, caves will generate
	float minCaveThreshold;
	float maxCaveThreshold;
};

extern WorldNoise worldNoise;
extern int seed;

void resetWorldNoise();
float lerp(float a, float b, float t);
void generateWorld(GameMap& gameMap, const int WIDTH, const int HEIGHT, int seed = DEFAULT_SEED, bool resetNoise = true);