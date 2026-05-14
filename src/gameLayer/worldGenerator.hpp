#pragma once
#include "gameMap.hpp"

#define DEFAULT_SEED 1234

struct WorldGen
{
	// Mountain settings
	// Noise generation settings
	int dirtMountainOctaves;
	float dirtMountainFrequency;
	int stoneMountainOctaves;
	float stoneMountainFrequency;
	// Terrain generation settings
	int minDirtMountainThickness;
	int maxDirtMountainThickness;
	int minStoneMountainStart;
	int maxStoneMountainStart;
	
	// Plain settings
	// Noise generation settings
	int dirtPlainOctaves;
	float dirtPlainFrequency;
	int stonePlainOctaves;
	float stonePlainFrequency;
	// Terrain generation settings
	int minDirtPlainThickness;
	int maxDirtPlainThickness;
	int minStonePlainStart;
	int maxStonePlainStart;

	// Biome settings
	int biomeOctaves;
	float biomeFrequency;
	float minDesertThreshold;
	float maxDesertThreshold;

	// Cave settings
	int caveOctaves;
	float caveFrequency;
	float minCaveThreshold;
	float maxCaveThreshold;
};

extern WorldGen worldGen;
extern int seed;

void resetWorldGen();
float lerp(float a, float b, float t);
void generateWorld(GameMap& gameMap, const int WIDTH, const int HEIGHT, int seed = DEFAULT_SEED, bool resetWorldGen = true);