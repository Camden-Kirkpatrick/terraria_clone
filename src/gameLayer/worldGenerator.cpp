#include "worldGenerator.hpp"
#include "randomStuff.hpp"
#include <FastNoiseSIMD.h>

void generateWorld(GameMap& gameMap, const int WIDTH, const int HEIGHT, int seed)
{   
    gameMap.create(WIDTH, HEIGHT);


    std::ranlux24_base rng(seed++);

    // One noise generator per terrain layer - different seeds produce independent shapes
    std::unique_ptr<FastNoiseSIMD> dirtNoiseGenrator(FastNoiseSIMD::NewFastNoiseSIMD());
    std::unique_ptr<FastNoiseSIMD> stoneNoiseGenrator(FastNoiseSIMD::NewFastNoiseSIMD());

    // Each generator gets a unique seed so their shapes don't match
    dirtNoiseGenrator->SetSeed(seed);
    stoneNoiseGenrator->SetSeed(seed);

    // Dirt: 1 octave = smooth gentle hills, higher frequency = shorter hills
    dirtNoiseGenrator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    dirtNoiseGenrator->SetFractalOctaves(1);
    dirtNoiseGenrator->SetFrequency(0.02);

    // Stone: 4 octaves = rougher jagged terrain, lower frequency = broader features
    stoneNoiseGenrator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    stoneNoiseGenrator->SetFractalOctaves(4);
    stoneNoiseGenrator->SetFrequency(0.01);

    // One float per world column, each will be filled with a value in roughly [-1, 1]
    float* dirtNoise = FastNoiseSIMD::GetEmptySet(WIDTH);
    float* stoneNoise = FastNoiseSIMD::GetEmptySet(WIDTH);

    // Sample a 1D horizontal slice (ySize=1, zSize=1) — one height value per column
    dirtNoiseGenrator->FillNoiseSet(dirtNoise, 0, 0, 0, WIDTH, 1, 1);
    stoneNoiseGenrator->FillNoiseSet(stoneNoise, 0, 0, 0, WIDTH, 1, 1);

    // Convert from [-1, 1] to [0, 1]
    for (int i = 0; i < WIDTH; i++)
    {
        dirtNoise[i] = (dirtNoise[i] + 1) / 2;
        stoneNoise[i] = (stoneNoise[i] + 1) / 2;

        //stoneNoise[i] = std::pow(stoneNoise[i], 2); //steeper mountains.
    }




    // Y coordinate bounds that clamp where the dirt/stone layer boundaries can settle (y=0 is the top of the world)
    //const int MIN_DIRT_HEIGHT = 50;
    //const int MAX_DIRT_HEIGHT = 90;
    //const int MIN_STONE_HEIGHT = 60;
    //const int MAX_STONE_HEIGHT = 120;

    const int MIN_DIRT_HEIGHT = -5;
    const int MAX_DIRT_HEIGHT = 35;
    const int MIN_STONE_HEIGHT = 80;
    const int MAX_STONE_HEIGHT = 170;
    const int GOLD_THRESHOLD = 60;
    const float GOLD_CHANCE = 0.01f;
   
    // Thresholds for when the block type changes
    int dirtHeight = 70;
    int stoneHeight = 90;

    // Go through every block in the map
    for (int x = 0; x < WIDTH; x++)
    {
        stoneHeight = MIN_STONE_HEIGHT + (MAX_STONE_HEIGHT - MIN_STONE_HEIGHT) * stoneNoise[x];
        dirtHeight = MIN_DIRT_HEIGHT + (MAX_DIRT_HEIGHT - MIN_DIRT_HEIGHT) * dirtNoise[x];
        dirtHeight = stoneHeight - dirtHeight;
        // Set the block type based on the current depth
        for (int y = 0; y < HEIGHT; y++)
        {
            Block b;

            // When y is above the stoneHight threshold, stone can generate
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

    

    FastNoiseSIMD::FreeNoiseSet(dirtNoise);
    FastNoiseSIMD::FreeNoiseSet(stoneNoise);
}


















//#include "worldGenerator.hpp"
//#include "randomStuff.hpp"
//#include <FastNoiseSIMD.h>
//
//
//void generateWorld(GameMap& gameMap, const int WIDTH, const int HEIGHT, int seed)
//{
//
//	const int w = 900;
//	const int h = 500;
//
//	gameMap.create(w, h);
//
//
//	std::ranlux24_base rng(seed++);
//
//
//	std::unique_ptr<FastNoiseSIMD> dirtNoiseGenrator(FastNoiseSIMD::NewFastNoiseSIMD());
//	std::unique_ptr<FastNoiseSIMD> stoneNoiseGenrator(FastNoiseSIMD::NewFastNoiseSIMD());
//
//
//	dirtNoiseGenrator->SetSeed(seed++);
//	stoneNoiseGenrator->SetSeed(seed++);
//
//	dirtNoiseGenrator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
//	dirtNoiseGenrator->SetFractalOctaves(1);
//	dirtNoiseGenrator->SetFrequency(0.02);
//
//	stoneNoiseGenrator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
//	stoneNoiseGenrator->SetFractalOctaves(4);
//	stoneNoiseGenrator->SetFrequency(0.01);
//
//	float* dirtNoise = FastNoiseSIMD::GetEmptySet(w);
//	float* stoneNoise = FastNoiseSIMD::GetEmptySet(w);
//
//	dirtNoiseGenrator->FillNoiseSet(dirtNoise, 0, 0, 0, w, 1, 1);
//	stoneNoiseGenrator->FillNoiseSet(stoneNoise, 0, 0, 0, w, 1, 1);
//
//	//convert from [-1 1] to [0 1]
//	for (int i = 0; i < w; i++)
//	{
//		dirtNoise[i] = (dirtNoise[i] + 1) / 2;
//		stoneNoise[i] = (stoneNoise[i] + 1) / 2;
//
//		//stoneNoise[i] = std::pow(stoneNoise[i], 2); //steeper mountains.
//	}
//
//	int dirtOffsetStart = -5;
//	int dirtOffsetEnd = 35;
//
//	int stoneHeightStart = 80;
//	int stoneHeightEnd = 170;
//
//
//	for (int x = 0; x < w; x++)
//	{
//
//		int stoneHeight = stoneHeightStart + (stoneHeightEnd - stoneHeightStart) * stoneNoise[x];
//		int dirtHeight = dirtOffsetStart + (dirtOffsetEnd - dirtOffsetStart) * dirtNoise[x];
//		dirtHeight = stoneHeight - dirtHeight;
//
//		for (int y = 0; y < h; y++)
//		{
//			Block b;
//
//			if (y > dirtHeight)
//			{
//				b.type = Block::dirt;
//			}
//
//			if (y == dirtHeight)
//			{
//				b.type = Block::grassBlock;
//			}
//
//			if (y >= stoneHeight)
//			{
//				b.type = Block::stone;
//			}
//
//			gameMap.getBlockUnsafe(x, y) = b;
//
//
//		}
//
//
//
//	}
//
//
//
//
//	FastNoiseSIMD::FreeNoiseSet(dirtNoise);
//	FastNoiseSIMD::FreeNoiseSet(stoneNoise);
//
//}