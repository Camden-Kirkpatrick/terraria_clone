#pragma once
#include "gameMap.hpp"

#define DEFAULT_SEED 1234

struct WorldGen
{
	int dirtMountainOctaves;       // 1 octave = smooth gentle hills
	float dirtMountainFrequency;   // Higher frequency = samples further apart on the noise curve = rapid value changes = narrow steep hills
	int stoneMountainOctaves;      // 4 octaves = more jagged terrain (also affects the dirt on top of it)
	float stoneMountainFrequency;  // Lower frequency = samples close together on the noise curve = slow value changes = gradual hills

	int minDirtMountainThickness;   // Minimum amount of dirt above stone
	int maxDirtMountainThickness;   // Maximum blocks of dirt above stone
	int minStoneMountainStart;      // Stone layer is at least this many blocks from the top
	int maxStoneMountainStart;      // The top of the stone layer is at most this many blocks from the top

	// Same settings as the ones above, but for plains instead of mountains
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