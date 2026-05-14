#pragma once
#include "gameMap.hpp"

#define DEFAULT_SEED 1234

struct WorldGen
{
	int dirtMountainOctaves;
	float dirtMountainFrequency;
	int stoneMountainOctaves;
	float stoneMountainFrequency;

	int minDirtMountainThickness;   // Minimum amount of dirt above stone
	int maxDirtMountainThickness;   // Maximum blocks of dirt above stone
	int minStoneMountainStart;      // Stone layer is at least this many blocks from the top
	int maxStoneMountainStart;      // The top of the stone layer is at most this many blocks from the top

	int dirtPlainOctaves;
	float dirtPlainFrequency;
	int stonePlainOctaves;
	float stonePlainFrequency;

	// Same settings as the ones above, but for plains instead of mountains
	int minDirtPlainThickness;
	int maxDirtPlainThickness;
	int minStonePlainStart;
	int maxStonePlainStart;

	int biomeOctaves;
	float biomeFrequency;

	int caveOctaves;
	float caveFrequency;

	// When the cave noise is in this range, caves will generate
	float minCaveThreshold;
	float maxCaveThreshold;
};

extern WorldGen worldGen;
extern int seed;

void resetWorldGen();
float lerp(float a, float b, float t);
void generateWorld(GameMap& gameMap, const int WIDTH, const int HEIGHT, int seed = DEFAULT_SEED, bool resetWorldGen = true);