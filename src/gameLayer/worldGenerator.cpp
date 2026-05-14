#include "worldGenerator.hpp"
#include "randomStuff.hpp"
#include <FastNoiseSIMD.h>

WorldNoise worldNoise;
int seed = DEFAULT_SEED;

void resetWorldNoise()
{
    worldNoise.dirtMountainOctaves = 1;
    worldNoise.dirtMountainFrequency = 0.02f;
    worldNoise.stoneMountainOctaves = 4;
    worldNoise.stoneMountainFrequency = 0.01f;

    worldNoise.minDirtMountainThickness = 1;
    worldNoise.maxDirtMountainThickness = 50;
    worldNoise.minStoneMountainStart = 330;
    worldNoise.maxStoneMountainStart = 400;

    worldNoise.dirtPlainOctaves = 1;
    worldNoise.dirtPlainFrequency = 0.0025f;
    worldNoise.stonePlainOctaves = 4;
    worldNoise.stonePlainFrequency = 0.005f;

    worldNoise.biomeOctaves = 1;
    worldNoise.biomeFrequency = 0.00025f;

    worldNoise.caveOctaves = 1;
    worldNoise.caveFrequency = 0.02f;

    worldNoise.minCaveThreshold = 0.65f;
    worldNoise.maxCaveThreshold = 0.8f;
}

float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

void generateWorld(GameMap& gameMap, const int WIDTH, const int HEIGHT, int seed, bool resetNoise)
{
    if (resetNoise)
        resetWorldNoise();

    gameMap.create(WIDTH, HEIGHT);

    std::ranlux24_base rng(seed++);

#pragma region generate_noise
    // Noise generators for different layers, biomes, and caves
    std::unique_ptr<FastNoiseSIMD> dirtNoiseGenerator(FastNoiseSIMD::NewFastNoiseSIMD());
    std::unique_ptr<FastNoiseSIMD> stoneNoiseGenerator(FastNoiseSIMD::NewFastNoiseSIMD());
    std::unique_ptr<FastNoiseSIMD> biomeNoiseGenerator(FastNoiseSIMD::NewFastNoiseSIMD());
    std::unique_ptr<FastNoiseSIMD> caveNoiseGenerator(FastNoiseSIMD::NewFastNoiseSIMD());

    // Each generator gets a unique seed so their shapes don't match
    dirtNoiseGenerator->SetSeed(seed++);
    stoneNoiseGenerator->SetSeed(seed++);
    biomeNoiseGenerator->SetSeed(seed++);
    caveNoiseGenerator->SetSeed(seed++);


    // Noise for mountains
    // Dirt:
    // 1 octave = smooth gentle hills
    // Higher frequency = samples further apart on the noise curve = rapid value changes = narrow steep hills
    dirtNoiseGenerator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    dirtNoiseGenerator->SetFractalOctaves(worldNoise.dirtMountainOctaves);
    dirtNoiseGenerator->SetFrequency(worldNoise.dirtMountainFrequency);

    // Stone:
    // 4 octaves = rougher jagged terrain
    // Lower frequency = samples closer together on the noise curve = gradual value changes = broad wide hills
    stoneNoiseGenerator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    stoneNoiseGenerator->SetFractalOctaves(worldNoise.stoneMountainOctaves);
    stoneNoiseGenerator->SetFrequency(worldNoise.stoneMountainFrequency);

    // One float per world column, each will be filled with a value in roughly [-1, 1]
    float* dirtMountainNoise = FastNoiseSIMD::GetEmptySet(WIDTH);
    float* stoneMountainNoise = FastNoiseSIMD::GetEmptySet(WIDTH);

    // Sample a 1D horizontal slice (ySize=1, zSize=1) - one height value per column
    dirtNoiseGenerator->FillNoiseSet(dirtMountainNoise, 0, 0, 0, WIDTH, 1, 1);
    stoneNoiseGenerator->FillNoiseSet(stoneMountainNoise, 0, 0, 0, WIDTH, 1, 1);


    // Noise for plains
    // Dirt:
    dirtNoiseGenerator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    dirtNoiseGenerator->SetFractalOctaves(worldNoise.dirtPlainOctaves);
    dirtNoiseGenerator->SetFrequency(worldNoise.dirtPlainFrequency);

    // Stone:
    stoneNoiseGenerator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    stoneNoiseGenerator->SetFractalOctaves(worldNoise.stonePlainOctaves);
    stoneNoiseGenerator->SetFrequency(worldNoise.stonePlainFrequency);

    float* dirtPlainNoise = FastNoiseSIMD::GetEmptySet(WIDTH);
    float* stonePlainNoise = FastNoiseSIMD::GetEmptySet(WIDTH);

    dirtNoiseGenerator->FillNoiseSet(dirtPlainNoise, 0, 0, 0, WIDTH, 1, 1);
    stoneNoiseGenerator->FillNoiseSet(stonePlainNoise, 0, 0, 0, WIDTH, 1, 1);


    // Noise for switching between biomes
    biomeNoiseGenerator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    biomeNoiseGenerator->SetFractalOctaves(worldNoise.biomeOctaves);
    // Lower frequency = larger biome regions, slower transitions between plains and mountains
    biomeNoiseGenerator->SetFrequency(worldNoise.biomeFrequency);

    float* biomeNoise = FastNoiseSIMD::GetEmptySet(WIDTH);

    biomeNoiseGenerator->FillNoiseSet(biomeNoise, 0, 0, 0, WIDTH, 1, 1);


    // Noise for caves
    caveNoiseGenerator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    caveNoiseGenerator->SetFractalOctaves(worldNoise.caveOctaves);
    caveNoiseGenerator->SetFrequency(worldNoise.caveFrequency);

    float* caveNoise = FastNoiseSIMD::GetEmptySet(WIDTH * HEIGHT);

    caveNoiseGenerator->FillNoiseSet(caveNoise, 0, 0, 0, HEIGHT, WIDTH, 1);


    // Noise output is in range [-1, 1], remap to [0, 1]
    for (int i = 0; i < WIDTH; i++)
    {
        dirtPlainNoise[i] = (dirtPlainNoise[i] + 1) / 2;
        stonePlainNoise[i] = (stonePlainNoise[i] + 1) / 2;

        dirtMountainNoise[i] = (dirtMountainNoise[i] + 1) / 2;
        stoneMountainNoise[i] = (stoneMountainNoise[i] + 1) / 2;

        biomeNoise[i] = (biomeNoise[i] + 1) / 2;
    }
    for (int i = 0; i < WIDTH * HEIGHT; i++)
        caveNoise[i] = (caveNoise[i] + 1) / 2;

    auto getCaveNoise = [&](int x, int y)
    {
        return caveNoise[WIDTH * y + x];
    };
#pragma endregion

#pragma region world_gen_constants
    //const int MIN_DIRT_MOUNTAIN_THICKNESS = 1;   // Minimum amount of dirt above stone
    //const int MAX_DIRT_MOUNTAIN_THICKNESS = 50;  // Maximum blocks of dirt above stone
    //const int MIN_STONE_MOUNTAIN_START = 330;    // Stone layer is at least 80 blocks from the top
    //const int MAX_STONE_MOUNTAIN_START = 400;    // The top of the stone layer is at most 150 blocks from the top

    const int MIN_DIRT_PLAIN_THICKNESS = 1;
    const int MAX_DIRT_PLAIN_THICKNESS = 5;
    const int MIN_STONE_PLAIN_START = 330;
    const int MAX_STONE_PLAIN_START = 335;

    const int ORE_THRESHOLD = 375;
    const float GOLD_CHANCE = 0.01f;
    const float IRON_CHANCE = 0.02f;
    const float COPPER_CHANCE = 0.03f;

    const int RUBY_THRESHOLD = 450;
    const float RUBY_CHANCE = 0.002f;

    const int CLAY_THRESHOLD = 355;
    const float CLAY_CHANCE = 0.8f;

    // When the biome noise is in this range, deserts will generate
    const float MIN_DESERT_THRESHOLD = 0.2f;
    const float MAX_DESERT_THRESHOLD = 0.4f;

    
    //float MIN_CAVE_THRESHOLD = 0.65f;
    //float MAX_CAVE_THRESHOLD = 0.8f;

#pragma endregion

    // Go through every block in the map
    for (int x = 0; x < WIDTH; x++)
    {
        // Lerp: find the heights for stone in the different biomes
        int stonePlainStart = lerp(MIN_STONE_PLAIN_START, MAX_STONE_PLAIN_START, stonePlainNoise[x]);
        int stoneMountainStart = lerp(worldNoise.minStoneMountainStart, worldNoise.maxStoneMountainStart, stoneMountainNoise[x]);
        // Lerp: find the thicknesses for dirt in the different biomes
        int dirtPlainThickness = lerp(MIN_DIRT_PLAIN_THICKNESS, MAX_DIRT_PLAIN_THICKNESS, dirtPlainNoise[x]);
        int dirtMountainThickness = lerp(worldNoise.minDirtMountainThickness, worldNoise.maxDirtMountainThickness, dirtMountainNoise[x]);

        // Lerp: find the stone height and dirt thickness based on the biome
        // Biome noise close to 0 generates plain-like terrain
        // Biome noise close to 1 generates mountain-like terrain
        int stoneStart = lerp(stonePlainStart, stoneMountainStart, biomeNoise[x]);
        int dirtThickness = lerp(dirtPlainThickness, dirtMountainThickness, biomeNoise[x]);

        int dirtStart = stoneStart - dirtThickness;

        // Set the block type based on the current depth
        for (int y = 0; y < HEIGHT; y++)
        {
            Block b;

#pragma region grasslands_biome
            // When y is deeper than the stone surface, stone can generate
            if (y > stoneStart && (biomeNoise[x] <= MIN_DESERT_THRESHOLD || biomeNoise[x] >= MAX_DESERT_THRESHOLD))
            {
                b.type = Block::stone;
                // Gold can generate further down in the stone layer
                if (y > ORE_THRESHOLD)
                {
                    // GOLD_CHANCE chance for gold to generate instead of stone
                    if (getRandomChance(rng, GOLD_CHANCE))
                        b.type = Block::gold;
                    // If gold doesn't generate, see if iron will
                    else if (getRandomChance(rng, IRON_CHANCE))
                        b.type = Block::iron;
                }
            }

            // When y is above the dirtHeight threshold, dirt can generate
            else if (y > dirtStart && (biomeNoise[x] <= MIN_DESERT_THRESHOLD || biomeNoise[x] >= MAX_DESERT_THRESHOLD))
            {
                b.type = Block::dirt;
                // Clay can generate further down in the dirt layer
                if (y > CLAY_THRESHOLD)
                {
                    // CLAY_CHANCE chance for clay to generate instead of dirt
                    if (getRandomChance(rng, CLAY_CHANCE))
                        b.type = Block::clay;
                }
            }
                
            // When y is exactly equal to the dirtHeight threshold, grass generates
            else if (y == dirtStart && (biomeNoise[x] <= MIN_DESERT_THRESHOLD || biomeNoise[x] >= MAX_DESERT_THRESHOLD))
                b.type = Block::grassBlock;
#pragma endregion

#pragma region desert_biome
            float distToEdge = 0.0f;
            float blendZone = 0.0f;
            float blendChance = 0.0f;

            if (biomeNoise[x] > MIN_DESERT_THRESHOLD && biomeNoise[x] < MAX_DESERT_THRESHOLD)
            {
                // How close are we the the nearest edge of the desert
                distToEdge = std::min(biomeNoise[x] - MIN_DESERT_THRESHOLD, MAX_DESERT_THRESHOLD - biomeNoise[x]);
                // This is the width (in noise units) of the band near each boundary where blending happens
                // With 0.015, only columns whose noise is within 0.015 of a boundary will receive any grassy-biome blocks
                blendZone = 0.015f;
                // Probability of placing grassy biome blocks instead of desert blocks.
                // High near the desert boundary, zero in the interior.
                // e.g. distToEdge=0.000 (boundary) -> chance=1.0, distToEdge=0.008 (halfway) -> chance=0.47, distToEdge=0.015+ (interior) -> chance=0.0
                blendChance = 1.0f - (distToEdge / blendZone);
            }

            // If we are in the stone layer and in the desert, use the correct blocks
            if (y > stoneStart && biomeNoise[x] > MIN_DESERT_THRESHOLD && biomeNoise[x] < MAX_DESERT_THRESHOLD)
            {
                // Stone can generate near biome edges
                if (getRandomChance(rng, blendChance))
                    b.type = Block::stone;
                else if (getRandomChance(rng, 0.5f))
                    b.type = Block::sand;
                else
                    b.type = Block::sandStone;

                // Rubies can generate deep in the stone layer
                if (y > RUBY_THRESHOLD)
                {
                    if (getRandomChance(rng, RUBY_CHANCE))
                        b.type = Block::sandRuby;
                }
                // Not deep enough for rubies, but other ores could still generate
                else if (y > ORE_THRESHOLD)
                {
                    // Other ores can generate near biome edges
                    if (getRandomChance(rng, blendChance))
                    {
                        if (getRandomChance(rng, GOLD_CHANCE))
                            b.type = Block::gold;
                        else if (getRandomChance(rng, IRON_CHANCE))
                            b.type = Block::iron;
                    }
                    // Copper can generate in the desert
                    else if (getRandomChance(rng, COPPER_CHANCE))
                        b.type = Block::copper;
                }
            }

            // If we are higher up in the desert, sand generates instead of dirt and grass
            else if (y >= dirtStart && biomeNoise[x] > MIN_DESERT_THRESHOLD && biomeNoise[x] < MAX_DESERT_THRESHOLD)
            {
                b.type = Block::sand;

                // Grass and dirt blocks can generate near biome edges
                if (getRandomChance(rng, blendChance))
                {
                    if (y == dirtStart)
                        b.type = Block::grassBlock;
                    else
                        b.type = Block::dirt;
                }
            }
                
#pragma endregion


            // Each block will use one of 4 random texture variations
            b.randIndex = getRandomInt(rng, 0, 3);

            // Store the block in the map
            gameMap.getBlockUnsafe(x, y) = b;

            // Use the correct background based on the block placed
            gameMap.getWallBlockUnsafe(x, y) = b;

            // Prevent caves from opening up to the void / edge of the map
            if (y == HEIGHT - 1 || x == 0 || x == WIDTH - 1) {}

#pragma region generate_caves
            // Cave generation
            else if (getCaveNoise(x, y) < worldNoise.maxCaveThreshold && getCaveNoise(x, y) > worldNoise.minCaveThreshold)
            {
                b.type = Block::air;
                gameMap.getBlockUnsafe(x, y) = b;

                // The background block shouldn't be air in caves, but the foreground block should be air
                Block background;
                background.randIndex = getRandomInt(rng, 0, 3);
                // If we are in the stone layer in the desert, use the correct background blocks
                if (y > stoneStart && biomeNoise[x] > MIN_DESERT_THRESHOLD && biomeNoise[x] < MAX_DESERT_THRESHOLD)
                {
                    if (getRandomChance(rng, blendChance))
                        background.type = Block::stone;
                    else if (getRandomChance(rng, 0.5f))
                        background.type = Block::sand;
                    else
                        background.type = Block::sandStone;

                    gameMap.getWallBlockUnsafe(x, y) = background;
                }
                // If we are in the stone layer in the grasslands, use the correct background block
                else if (y > stoneStart)
                {
                    background.type = Block::stone;
                    gameMap.getWallBlockUnsafe(x, y) = background;
                }
            }
#pragma endregion
        }
    }

    // Free resources
    FastNoiseSIMD::FreeNoiseSet(dirtPlainNoise);
    FastNoiseSIMD::FreeNoiseSet(stonePlainNoise);
    FastNoiseSIMD::FreeNoiseSet(dirtMountainNoise);
    FastNoiseSIMD::FreeNoiseSet(stoneMountainNoise);
    FastNoiseSIMD::FreeNoiseSet(biomeNoise);
    FastNoiseSIMD::FreeNoiseSet(caveNoise);
}