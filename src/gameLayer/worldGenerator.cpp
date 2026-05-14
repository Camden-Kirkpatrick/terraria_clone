#include "worldGenerator.hpp"
#include "randomStuff.hpp"
#include <FastNoiseSIMD.h>

WorldGen worldGen;
int seed = DEFAULT_SEED;

void resetWorldGen()
{
    worldGen.dirtMountainOctaves = 1;
    worldGen.dirtMountainFrequency = 0.02f;
    worldGen.stoneMountainOctaves = 4;
    worldGen.stoneMountainFrequency = 0.01f;

    worldGen.minDirtMountainThickness = 1;
    worldGen.maxDirtMountainThickness = 50;
    worldGen.minStoneMountainStart = 330;
    worldGen.maxStoneMountainStart = 400;

    worldGen.dirtPlainOctaves = 1;
    worldGen.dirtPlainFrequency = 0.0025f;
    worldGen.stonePlainOctaves = 4;
    worldGen.stonePlainFrequency = 0.005f;

    worldGen.minDirtPlainThickness = 1;
    worldGen.maxDirtPlainThickness = 6;
    worldGen.minStonePlainStart = 330;
    worldGen.maxStonePlainStart = 335;

    worldGen.biomeOctaves = 1;
    worldGen.biomeFrequency = 0.00025f;

    worldGen.caveOctaves = 1;
    worldGen.caveFrequency = 0.02f;

    worldGen.minCaveThreshold = 0.65f;
    worldGen.maxCaveThreshold = 0.8f;
}

float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

void generateWorld(GameMap& gameMap, const int WIDTH, const int HEIGHT, int seed, bool resetWrldGen)
{
    if (resetWrldGen)
        resetWorldGen();

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
    dirtNoiseGenerator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    dirtNoiseGenerator->SetFractalOctaves(worldGen.dirtMountainOctaves);
    dirtNoiseGenerator->SetFrequency(worldGen.dirtMountainFrequency);

    // Stone:
    stoneNoiseGenerator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    stoneNoiseGenerator->SetFractalOctaves(worldGen.stoneMountainOctaves);
    stoneNoiseGenerator->SetFrequency(worldGen.stoneMountainFrequency);

    // One float per world column, each will be filled with a value in roughly [-1, 1]
    float* dirtMountainNoise = FastNoiseSIMD::GetEmptySet(WIDTH);
    float* stoneMountainNoise = FastNoiseSIMD::GetEmptySet(WIDTH);

    // Sample a 1D horizontal slice (ySize=1, zSize=1) - one height value per column
    dirtNoiseGenerator->FillNoiseSet(dirtMountainNoise, 0, 0, 0, WIDTH, 1, 1);
    stoneNoiseGenerator->FillNoiseSet(stoneMountainNoise, 0, 0, 0, WIDTH, 1, 1);


    // Noise for plains
    // Dirt:
    dirtNoiseGenerator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    dirtNoiseGenerator->SetFractalOctaves(worldGen.dirtPlainOctaves);
    dirtNoiseGenerator->SetFrequency(worldGen.dirtPlainFrequency);

    // Stone:
    stoneNoiseGenerator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    stoneNoiseGenerator->SetFractalOctaves(worldGen.stonePlainOctaves);
    stoneNoiseGenerator->SetFrequency(worldGen.stonePlainFrequency);

    float* dirtPlainNoise = FastNoiseSIMD::GetEmptySet(WIDTH);
    float* stonePlainNoise = FastNoiseSIMD::GetEmptySet(WIDTH);

    dirtNoiseGenerator->FillNoiseSet(dirtPlainNoise, 0, 0, 0, WIDTH, 1, 1);
    stoneNoiseGenerator->FillNoiseSet(stonePlainNoise, 0, 0, 0, WIDTH, 1, 1);


    // Noise for switching between biomes
    biomeNoiseGenerator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    biomeNoiseGenerator->SetFractalOctaves(worldGen.biomeOctaves);
    // Lower frequency = larger biome regions, slower transitions between plains and mountains
    biomeNoiseGenerator->SetFrequency(worldGen.biomeFrequency);

    float* biomeNoise = FastNoiseSIMD::GetEmptySet(WIDTH);

    biomeNoiseGenerator->FillNoiseSet(biomeNoise, 0, 0, 0, WIDTH, 1, 1);


    // Noise for caves
    caveNoiseGenerator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    caveNoiseGenerator->SetFractalOctaves(worldGen.caveOctaves);
    caveNoiseGenerator->SetFrequency(worldGen.caveFrequency);

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

#pragma endregion

    // Go through every block in the map
    for (int x = 0; x < WIDTH; x++)
    {
        // Lerp: find the heights for the stone layer
        int stonePlainStart = lerp(worldGen.minStonePlainStart, worldGen.maxStonePlainStart, stonePlainNoise[x]);
        int stoneMountainStart = lerp(worldGen.minStoneMountainStart, worldGen.maxStoneMountainStart, stoneMountainNoise[x]);
        // Lerp: find the thicknesses for the dirt layer
        int dirtPlainThickness = lerp(worldGen.minDirtPlainThickness, worldGen.maxDirtPlainThickness, dirtPlainNoise[x]);
        int dirtMountainThickness = lerp(worldGen.minDirtMountainThickness, worldGen.maxDirtMountainThickness, dirtMountainNoise[x]);

        // Lerp: find the stone height and dirt thickness based on the biome
        // Biome noise close to 0 generates plain-like terrain
        // Biome noise close to 1 generates mountain-like terrain
        int stoneStart = lerp(stonePlainStart, stoneMountainStart, biomeNoise[x]);
        int dirtThickness = lerp(dirtPlainThickness, dirtMountainThickness, biomeNoise[x]);
        // Dirt generates dirtThickness blocks above the stone layer
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
                    // If gold doesn't generate, iron has a chance to
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
            else if (getCaveNoise(x, y) < worldGen.maxCaveThreshold && getCaveNoise(x, y) > worldGen.minCaveThreshold)
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