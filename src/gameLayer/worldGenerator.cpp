#include "worldGenerator.hpp"
#include "randomStuff.hpp"
#include <FastNoiseSIMD.h>

WorldNoise worldNoise;
int seed = DEFAULT_SEED;

float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

void generateWorld(GameMap& gameMap, const int WIDTH, const int HEIGHT, int seed, bool resetNoise)
{   
    if (resetNoise)
    {
        worldNoise.dirtMountainOctaves = 1;
        worldNoise.dirtMountainFrequency = 0.02f;
        worldNoise.stoneMountainOctaves = 4;
        worldNoise.stoneMountainFrequency = 0.01f;

        worldNoise.dirtPlainOctaves = 1;
        worldNoise.dirtPlainFrequency = 0.0025f;
        worldNoise.stonePlainOctaves = 1;
        worldNoise.stonePlainFrequency = 0.005f;

        worldNoise.biomeOctaves = 1;
        worldNoise.biomeFrequency = 0.00025f;
    }

    gameMap.create(WIDTH, HEIGHT);

    std::ranlux24_base rng(seed++);

    // One noise generator per terrain layer - different seeds produce independent shapes
    std::unique_ptr<FastNoiseSIMD> dirtNoiseGenerator(FastNoiseSIMD::NewFastNoiseSIMD());
    std::unique_ptr<FastNoiseSIMD> stoneNoiseGenerator(FastNoiseSIMD::NewFastNoiseSIMD());
    std::unique_ptr<FastNoiseSIMD> biomeNoiseGenerator(FastNoiseSIMD::NewFastNoiseSIMD());

    // Each generator gets a unique seed so their shapes don't match
    dirtNoiseGenerator->SetSeed(seed++);
    stoneNoiseGenerator->SetSeed(seed++);
    biomeNoiseGenerator->SetSeed(seed++);
    

#pragma region generate_noise
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

    // Sample a 1D horizontal slice (ySize=1, zSize=1) — one height value per column
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

    // Noise output is in range [-1, 1], remap to [0, 1]
    for (int i = 0; i < WIDTH; i++)
    {
        dirtPlainNoise[i] = (dirtPlainNoise[i] + 1) / 2;
        stonePlainNoise[i] = (stonePlainNoise[i] + 1) / 2;

        dirtMountainNoise[i] = (dirtMountainNoise[i] + 1) / 2;
        stoneMountainNoise[i] = (stoneMountainNoise[i] + 1) / 2;

        biomeNoise[i] = (biomeNoise[i] + 1) / 2;
    }

#pragma endregion


    const int MIN_DIRT_MOUNTAIN_THICKNESS = -5;  // Negative allows stone to poke through dirt layer
    const int MAX_DIRT_MOUNTAIN_THICKNESS = 50;  // Maximum blocks of dirt above stone
    const int MIN_STONE_MOUNTAIN_HEIGHT = 80;    // Stone layer is at least 80 blocks from the top
    const int MAX_STONE_MOUNTAIN_HEIGHT = 150;   // The top of the stone layer is at most 150 blocks from the top

    const int MIN_DIRT_PLAIN_THICKNESS = 0;
    const int MAX_DIRT_PLAIN_THICKNESS = 5;
    const int MIN_STONE_PLAIN_HEIGHT = 80;
    const int MAX_STONE_PLAIN_HEIGHT = 85;

    const int GOLD_THRESHOLD = 60;      // Gold can generate 60 blocks from the top of the stone layer
    const float GOLD_CHANCE = 0.01f;

    // Go through every block in the map
    for (int x = 0; x < WIDTH; x++)
    {

        // Lerp: find the heights for stone in the different biomes
        int stonePlainHeight = lerp(MIN_STONE_PLAIN_HEIGHT, MAX_STONE_PLAIN_HEIGHT, stonePlainNoise[x]);
        int stoneMountainHeight = lerp(MIN_STONE_MOUNTAIN_HEIGHT, MAX_STONE_MOUNTAIN_HEIGHT, stoneMountainNoise[x]);
        // Lerp: find the thicknesses for dirt in the different biomes
        int dirtPlainThickness = lerp(MIN_DIRT_PLAIN_THICKNESS, MAX_DIRT_PLAIN_THICKNESS, dirtPlainNoise[x]);
        int dirtMountainThickness = lerp(MIN_DIRT_MOUNTAIN_THICKNESS, MAX_DIRT_MOUNTAIN_THICKNESS, dirtMountainNoise[x]);
         
        // Lerp: find the stone height and dirt thickness based on the biome
        int stoneHeight = lerp(stonePlainHeight, stoneMountainHeight, biomeNoise[x]);
        int dirtThickness = lerp(dirtPlainThickness, dirtMountainThickness, biomeNoise[x]);
        
        int dirtHeight = stoneHeight - dirtThickness;

        // Set the block type based on the current depth
        for (int y = 0; y < HEIGHT; y++)
        {
            Block b;

            // When y is deeper than the stone surface, stone can generate
            if (y > stoneHeight)
            {
                // Gold can generate further down in the stone layer
                if (y > stoneHeight + GOLD_THRESHOLD)
                {
                    // GOLD_CHANCE chance for gold to generate instead of stone
                    if (getRandomChance(rng, GOLD_CHANCE))
                        b.type = Block::gold;
                    else
                        b.type = Block::stone;
                }
                // Not deep enough to generate gold 
                else
                    b.type = Block::stone; 
            }

            // When y is above the dirtHeight threshold, dirt can generate
            else if (y > dirtHeight)   b.type = Block::dirt;
            // When y is exactly equal to the dirtHeight threshold, grass generates
            else if (y == dirtHeight)  b.type = Block::grassBlock;

            // Each block will use a random texture variation.
            b.randIndex = std::rand() % 4;

            // Set the map to use the correct block
            gameMap.getBlockUnsafe(x, y) = b;

            // Use the correct background based on the block placed
            gameMap.getWallBlockUnsafe(x, y) = b;

            // Ensure that gold has a stone background behind it
            if (b.type == Block::gold)
                gameMap.getWallBlockUnsafe(x, y).type = Block::stone;
        }
    }

    

    FastNoiseSIMD::FreeNoiseSet(dirtPlainNoise);
    FastNoiseSIMD::FreeNoiseSet(stonePlainNoise);
    FastNoiseSIMD::FreeNoiseSet(dirtMountainNoise);
    FastNoiseSIMD::FreeNoiseSet(stoneMountainNoise);
    FastNoiseSIMD::FreeNoiseSet(biomeNoise);
}