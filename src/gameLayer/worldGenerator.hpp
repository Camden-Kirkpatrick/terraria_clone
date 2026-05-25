#pragma once
#include "gameMap.hpp"

#define DEFAULT_WORLD_WIDTH 10000
#define DEFAULT_WORLD_HEIGHT 500
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

	float terrainBlendZone;
	
	// Plain settings
	// Noise generation settings
	float plainThreshold;
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

	// Desert settings
	float minDesertThreshold;
	float maxDesertThreshold;
	int desertOctaves;
	float desertFrequency;
	float desertBlendZone;

	// Cave settings
	bool generateCaves;
	int caveOctaves;
	float caveFrequency;
	float minCaveThreshold;
	float maxCaveThreshold;

	// Special block settings
	// Ores
	int oreThreshold;
	float goldChance;
	float ironChance;
	float copperChance;
	// Rubies
	int rubyThreshold;
	float rubyChance;
	// Clay
	int clayThreshold;
	float clayChance;

	// Tunnel worm settings
	bool generateWorms;
	int curNumWorms;
	int minNumWorms;
	int maxNumWorms;
	int minWormWidth;
	int maxWormWidth;
	float minWormTurnAngle;
	float maxWormTurnAngle;
};

extern WorldGen worldGen;
extern int worldWidth;
extern int worldHeight;
extern int seed;

extern std::vector<float> savedBiomeNoise;

void resetWorldGen();
void flatWorld();
float lerp(float a, float b, float t);
void generateWorld(GameMap& gameMap, const int WIDTH = DEFAULT_WORLD_WIDTH, const int HEIGHT = DEFAULT_WORLD_HEIGHT, int seed = DEFAULT_SEED, bool resetWorldGen = true);