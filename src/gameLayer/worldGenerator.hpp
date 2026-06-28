#pragma once
#include "gameMap.hpp"
#include <cstdint>

#define DEFAULT_WORLD_WIDTH 10000
#define DEFAULT_WORLD_HEIGHT 500
#define DEFAULT_SEED 2112

// Used to calculate min/maxNumWorms
#define MAX_WORM_DIVISOR 100
#define MIN_WORM_DIVISOR 400

enum class Biome : uint8_t
{
	Grasslands,
	Desert,
	Tundra
};

struct WorldGen
{
	// Terrain settings
	int terrainOctaves;
	float terrainFrequency;
	float terrainBlendZone;

	// Mountain settings
	// Noise generation settings
	int dirtMountainOctaves;
	float dirtMountainFrequency;
	int stoneMountainOctaves;
	float stoneMountainFrequency;
	// Mountain generation settings
	int minDirtMountainThickness;
	int maxDirtMountainThickness;
	int minStoneMountainStart;
	int maxStoneMountainStart;
	
	// Plain settings
	// Noise generation settings
	float plainThreshold;
	int dirtPlainOctaves;
	float dirtPlainFrequency;
	int stonePlainOctaves;
	float stonePlainFrequency;
	// Plain generation settings
	int minDirtPlainThickness;
	int maxDirtPlainThickness;
	int minStonePlainStart;
	int maxStonePlainStart;

	// Biome settings
	int biomeOctaves;
	float biomeFrequency;
	int biomeBlendRadius;
	// Desert settings
	float minDesertThreshold;
	float maxDesertThreshold;
	// Tundra settings
	float minTundraThreshold;
	float maxTundraThreshold;

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
	int minWormLength;
	int maxWormLength;
	int minWormWidth;
	int maxWormWidth;
	float minWormTurnAngle;
	float maxWormTurnAngle;

	// Tree settings
	bool generateTrees;
	float treeSpawnChance;
};

extern WorldGen worldGen;
extern int worldWidth;
extern int worldHeight;
extern int seed;

extern std::vector<float> savedTerrainNoise;

void resetWorldGen();
void flatWorld();
float lerp(float a, float b, float t);
float invLerp(float a, float b, float v);
float terrainBlend(float terrainNoise);
Biome biomeFromNoise(float biomeNoise);
void generateWorld(GameMap& gameMap, const int WIDTH = DEFAULT_WORLD_WIDTH, const int HEIGHT = DEFAULT_WORLD_HEIGHT, int seed = DEFAULT_SEED, bool resetWorldGen = true);