#include "worldGenerator.hpp"
#include "randomStuff.hpp"
#include <FastNoiseSIMD.h>

float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

void generateWorld(GameMap& gameMap, const int WIDTH, const int HEIGHT, int seed)
{   
    gameMap.create(WIDTH, HEIGHT);


    std::ranlux24_base rng(seed++);

    // One noise generator per terrain layer - different seeds produce independent shapes
    std::unique_ptr<FastNoiseSIMD> dirtNoiseGenrator(FastNoiseSIMD::NewFastNoiseSIMD());
    std::unique_ptr<FastNoiseSIMD> stoneNoiseGenrator(FastNoiseSIMD::NewFastNoiseSIMD());
    std::unique_ptr<FastNoiseSIMD> biomeNoiseGenrator(FastNoiseSIMD::NewFastNoiseSIMD());

    // Each generator gets a unique seed so their shapes don't match
    dirtNoiseGenrator->SetSeed(seed++);
    stoneNoiseGenrator->SetSeed(seed++);
    biomeNoiseGenrator->SetSeed(seed++);
    

#pragma region generate_noise
    // Noise for mountains
    // Dirt:
    // 1 octave = smooth gentle hills
    // Higher frequency = samples further apart on the noise curve = rapid value changes = narrow steep hills
    dirtNoiseGenrator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    dirtNoiseGenrator->SetFractalOctaves(1);
    dirtNoiseGenrator->SetFrequency(0.02);

    // Stone:
    // 4 octaves = rougher jagged terrain
    // Lower frequency = samples closer together on the noise curve = gradual value changes = broad wide hills
    stoneNoiseGenrator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    stoneNoiseGenrator->SetFractalOctaves(4);
    stoneNoiseGenrator->SetFrequency(0.01);

    // One float per world column, each will be filled with a value in roughly [-1, 1]
    float* dirtMountainNoise = FastNoiseSIMD::GetEmptySet(WIDTH);
    float* stoneMountainNoise = FastNoiseSIMD::GetEmptySet(WIDTH);

    // Sample a 1D horizontal slice (ySize=1, zSize=1) — one height value per column
    dirtNoiseGenrator->FillNoiseSet(dirtMountainNoise, 0, 0, 0, WIDTH, 1, 1);
    stoneNoiseGenrator->FillNoiseSet(stoneMountainNoise, 0, 0, 0, WIDTH, 1, 1);


    // Noise for plains
    // Dirt:
    dirtNoiseGenrator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    dirtNoiseGenrator->SetFractalOctaves(1);
    dirtNoiseGenrator->SetFrequency(0.0025);

    // Stone:
    stoneNoiseGenrator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    stoneNoiseGenrator->SetFractalOctaves(1);
    stoneNoiseGenrator->SetFrequency(0.005);

    float* dirtPlainNoise = FastNoiseSIMD::GetEmptySet(WIDTH);
    float* stonePlainNoise = FastNoiseSIMD::GetEmptySet(WIDTH);

    dirtNoiseGenrator->FillNoiseSet(dirtPlainNoise, 0, 0, 0, WIDTH, 1, 1);
    stoneNoiseGenrator->FillNoiseSet(stonePlainNoise, 0, 0, 0, WIDTH, 1, 1);


    // Noise for switching between biomes
    biomeNoiseGenrator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    biomeNoiseGenrator->SetFractalOctaves(1);
    // Lower frequency = larger biome regions, slower transitions between plains and mountains
    biomeNoiseGenrator->SetFrequency(0.00025);

    float* biomeNoise = FastNoiseSIMD::GetEmptySet(WIDTH);

    biomeNoiseGenrator->FillNoiseSet(biomeNoise, 0, 0, 0, WIDTH, 1, 1);

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
        //// Lerp: find where the stone surface sits in this column (y=0 is top, so smaller = higher up)
        //int stoneHeight = lerp(MIN_STONE_PLAIN_HEIGHT, MAX_STONE_PLAIN_HEIGHT, stonePlainNoise[x]);
        //// Lerp: find how many blocks thick the dirt layer is for this column
        //int dirtThickness = lerp(MIN_DIRT_PLAIN_THICKNESS, MAX_DIRT_PLAIN_THICKNESS, dirtPlainNoise[x]);
        //// The dirt surface sits above the stone surface by dirtThickness blocks (subtract because y=0 is top)
        //int dirtHeight = stoneHeight - dirtThickness;

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