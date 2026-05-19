#include "worldGenerator.hpp"
#include "randomStuff.hpp"
#include <FastNoiseSIMD.h>

WorldGen worldGen;
int worldWidth = DEFAULT_WORLD_WIDTH;
int worldHeight = DEFAULT_WORLD_HEIGHT;
int seed = DEFAULT_SEED;

void resetWorldGen()
{
    worldWidth = DEFAULT_WORLD_WIDTH;
    worldHeight = DEFAULT_WORLD_HEIGHT;

    // Mountain settings
    // Noise generation settings
    worldGen.dirtMountainOctaves = 1;              // 1 octave = smooth gentle hills
    worldGen.dirtMountainFrequency = 0.02f;        // Higher frequency = samples further apart on the noise curve = rapid value changes = narrow steep hills
    worldGen.stoneMountainOctaves = 4;             // 4 octaves = more jagged terrain (also affects the dirt on top of it)
    worldGen.stoneMountainFrequency = 0.01f;       // Lower frequency = samples close together on the noise curve = slow value changes = gradual hills
    // Terrain generation settings
    worldGen.minDirtMountainThickness = 1;         // Minimum amount of dirt above stone
    worldGen.maxDirtMountainThickness = 50;        // Maximum blocks of dirt above stone
    worldGen.minStoneMountainStart = 330;          // Stone layer is at least this many blocks from the top
    worldGen.maxStoneMountainStart = 400;          // The top of the stone layer is at most this many blocks from the top

    // Plain settings
    // Noise generation settings
    worldGen.dirtPlainOctaves = 1;
    worldGen.dirtPlainFrequency = 0.0025f;
    worldGen.stonePlainOctaves = 4;
    worldGen.stonePlainFrequency = 0.005f;
    // Terrain generation settings
    worldGen.minDirtPlainThickness = 1;
    worldGen.maxDirtPlainThickness = 6;
    worldGen.minStonePlainStart = 330;
    worldGen.maxStonePlainStart = 335;

    // Biome settings
    worldGen.biomeOctaves = 1;
    worldGen.biomeFrequency = 0.00025f;
    // When the biome noise is in this range, deserts will generate
    worldGen.minDesertThreshold = 0.2f;
    worldGen.maxDesertThreshold = 0.4f;
    // This is the width (in noise units) of the band near each boundary where blending happens
    // With 0.015, only columns whose noise is within 0.015 of a boundary will receive any grassy-biome blocks
    worldGen.blendZone = 0.015f;

    // Cave settings
    worldGen.generateCaves = true;
    worldGen.caveOctaves = 6;
    worldGen.caveFrequency = 0.004f;
    // When the cave noise is in this range, caves will generate
    worldGen.minCaveThreshold = 0.65f;
    worldGen.maxCaveThreshold = 0.8f;

    // Special block settings
    // Ores
    worldGen.oreThreshold = 375;
    worldGen.goldChance = 0.01f;
    worldGen.ironChance = 0.02f;
    worldGen.copperChance = 0.03f;
    // Rubies
    worldGen.rubyThreshold = 450;
    worldGen.rubyChance = 0.002f;
    // Clay
    worldGen.clayThreshold = 355;
    worldGen.clayChance = 0.8f;

    // Tunnel worm settings
    worldGen.generateWorms = true;
    worldGen.curNumWorms = 0;
    worldGen.minNumWorms = 25;
    worldGen.maxNumWorms = 100;
    worldGen.minWormWidth = 1;
    worldGen.maxWormWidth = 5;
    worldGen.minWormTurnAngle = -0.2f;
    worldGen.maxWormTurnAngle = 0.2f;
}

void flatWorld()
{
    worldGen.minDirtMountainThickness = 1;
    worldGen.maxDirtMountainThickness = 1;
    worldGen.minStoneMountainStart = 330;
    worldGen.maxStoneMountainStart = 330;

    worldGen.minDirtPlainThickness = 1;
    worldGen.maxDirtPlainThickness = 1;
    worldGen.minStonePlainStart = 330;
    worldGen.maxStonePlainStart = 330;
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
    // Two independent cave shape noises. The final cave noise at each tile is a
    // lerp between these two, weighted by caveSelectorNoise. Each is sampled
    // across the full WIDTH x HEIGHT grid (caves are 2D, unlike terrain heights).
    //
    // Noise #1: low frequency + high octaves = large caves with fine, rough edges.
    // Noise #2: high frequency + medium octaves = small, dense, scattered caves.
    // Different frequencies decorrelate the two shapes even though they share
    // the same FastNoise generator/seed.

    // Noise #1
    caveNoiseGenerator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    caveNoiseGenerator->SetFractalOctaves(worldGen.caveOctaves);
    caveNoiseGenerator->SetFrequency(worldGen.caveFrequency);

    float* caveNoise1 = FastNoiseSIMD::GetEmptySet(WIDTH * HEIGHT);

    caveNoiseGenerator->FillNoiseSet(caveNoise1, 0, 0, 0, HEIGHT, WIDTH, 1);

    // Noise #2
    caveNoiseGenerator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    caveNoiseGenerator->SetFractalOctaves(3);
    caveNoiseGenerator->SetFrequency(0.02f);

    float* caveNoise2 = FastNoiseSIMD::GetEmptySet(WIDTH * HEIGHT);

    caveNoiseGenerator->FillNoiseSet(caveNoise2, 0, 0, 0, HEIGHT, WIDTH, 1);

    // Cave selector noise
    // Slow-frequency selector that picks which cave shape dominates in each region.
    // Lower frequency than the cave noises so a region commits to one style across
    // many tiles instead of flickering. Value near 0 = mostly caveNoise1's shape,
    // value near 1 = mostly caveNoise2's shape, in-between = smooth blend.
    caveNoiseGenerator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    caveNoiseGenerator->SetFractalOctaves(1);
    caveNoiseGenerator->SetFrequency(0.0025f);

    float* caveSelectorNoise = FastNoiseSIMD::GetEmptySet(WIDTH * HEIGHT);

    caveNoiseGenerator->FillNoiseSet(caveSelectorNoise, 0, 0, 0, HEIGHT, WIDTH, 1);


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
    {
        caveNoise1[i] = (caveNoise1[i] + 1) / 2;
        caveNoise2[i] = (caveNoise2[i] + 1) / 2;
        caveSelectorNoise[i] = (caveSelectorNoise[i] + 1) / 2;
    }

    auto getCaveNoise1 = [&](int x, int y)
        {
            return caveNoise1[WIDTH * y + x];
        };
    auto getCaveNoise2 = [&](int x, int y)
        {
            return caveNoise2[WIDTH * y + x];
        };
    auto getCaveSelectorNoise = [&](int x, int y)
        {
            return caveSelectorNoise[WIDTH * y + x];
        };
    // Blend the two cave shapes per-tile using the selector as the lerp weight.
    // Then a single band threshold on the result carves the actual caves.
    auto getFinalCaveNoise = [&](int x, int y)
        {
            return lerp(getCaveNoise1(x, y), getCaveNoise2(x, y), getCaveSelectorNoise(x, y));
        };
#pragma endregion

    // Go through every block in the map
    for (int x = 0; x < WIDTH; x++)
    {
#pragma region linerar_interpolation
        // Lerp: find the heights for the stone layer
        int stonePlainStart = lerp(worldGen.minStonePlainStart, worldGen.maxStonePlainStart, stonePlainNoise[x]);
        int stoneMountainStart = lerp(worldGen.minStoneMountainStart, worldGen.maxStoneMountainStart, stoneMountainNoise[x]);
        // Lerp: find the thicknesses for the dirt layer
        int dirtPlainThickness = lerp(worldGen.minDirtPlainThickness, worldGen.maxDirtPlainThickness, dirtPlainNoise[x]);
        int dirtMountainThickness = lerp(worldGen.minDirtMountainThickness, worldGen.maxDirtMountainThickness, dirtMountainNoise[x]);

        // NOTE: Plain and mountain settings are NOT isolated to their own biomes.
        // Because lerp(a, b, t) = a*(1-t) + b*t, both inputs always contribute to the result
        // unless biomeNoise[x] is exactly 0 or 1 (which essentially never happens with simplex noise).
        // So changing a "plain" setting still affects mountain columns (weighted by 1 - biomeNoise[x]),
        // and changing a "mountain" setting still affects plain columns (weighted by biomeNoise[x]).
        // The effect is more visible for mountain settings because their min/max ranges are much
        // wider than the plain ranges (e.g. stoneMountainStart spans 70 blocks vs 5 for stonePlainStart),
        // so the same blend weight produces a larger absolute shift.
        // The same leak applies to the plain/mountain noise frequency and octaves: they only shape
        // their own noise array, but those arrays feed back into this lerp and bleed across biomes.

        // Lerp: find the stone height and dirt thickness based on the biome
        // Biome noise close to 0 generates plain-like terrain
        // Biome noise close to 1 generates mountain-like terrain
        int stoneStart = lerp(stonePlainStart, stoneMountainStart, biomeNoise[x]);
        int dirtThickness = lerp(dirtPlainThickness, dirtMountainThickness, biomeNoise[x]);
        // Dirt generates dirtThickness blocks above the stone layer
        int dirtStart = stoneStart - dirtThickness;




        //if (biomeNoise[x] < 0.5f)
        //{
        //    stoneStart = lerp(worldGen.minStonePlainStart, worldGen.maxStonePlainStart, stonePlainNoise[x]);
        //    dirtThickness = lerp(worldGen.minDirtPlainThickness, worldGen.maxDirtPlainThickness, dirtPlainNoise[x]);
        //}
        //else
        //{
        //    stoneStart = lerp(worldGen.minStoneMountainStart, worldGen.maxStoneMountainStart, stoneMountainNoise[x]);
        //    dirtThickness = lerp(worldGen.minDirtMountainThickness, worldGen.maxDirtMountainThickness, dirtMountainNoise[x]);
        //}
        //dirtStart = stoneStart - dirtThickness;
            





#pragma endregion

        // Set the block type based on the current depth
        for (int y = 0; y < HEIGHT; y++)
        {
            Block b;

#pragma region grasslands_biome
            // When y is deeper than the stone surface, stone can generate
            if (y > stoneStart && (biomeNoise[x] <= worldGen.minDesertThreshold || biomeNoise[x] >= worldGen.maxDesertThreshold))
            {
                b.type = Block::stone;
                // Gold can generate further down in the stone layer
                if (y > worldGen.oreThreshold)
                {
                    // worldGen.goldChance chance for gold to generate instead of stone
                    if (getRandomChance(rng, worldGen.goldChance))
                        b.type = Block::gold;
                    // If gold doesn't generate, iron has a chance to
                    else if (getRandomChance(rng, worldGen.ironChance))
                        b.type = Block::iron;
                }
            }

            // When y is above the dirtHeight threshold, dirt can generate
            else if (y > dirtStart && (biomeNoise[x] <= worldGen.minDesertThreshold || biomeNoise[x] >= worldGen.maxDesertThreshold))
            {
                b.type = Block::dirt;
                // Clay can generate further down in the dirt layer
                if (y > worldGen.clayThreshold)
                {
                    // worldGen.clayChance chance for clay to generate instead of dirt
                    if (getRandomChance(rng, worldGen.clayChance))
                        b.type = Block::clay;
                }
            }

            // When y is exactly equal to the dirtHeight threshold, grass generates
            else if (y == dirtStart && (biomeNoise[x] <= worldGen.minDesertThreshold || biomeNoise[x] >= worldGen.maxDesertThreshold))
                b.type = Block::grassBlock;
#pragma endregion

#pragma region desert_biome
            float distToEdge = 0.0f;
            float blendChance = 0.0f;

            if (biomeNoise[x] > worldGen.minDesertThreshold && biomeNoise[x] < worldGen.maxDesertThreshold)
            {
                // How close are we to the nearest edge of the desert
                distToEdge = std::min(biomeNoise[x] - worldGen.minDesertThreshold, worldGen.maxDesertThreshold - biomeNoise[x]);
                // Probability of placing grassy biome blocks instead of desert blocks.
                // High near the desert boundary, zero in the interior.
                // e.g. distToEdge=0.000 (boundary) -> chance=1.0, distToEdge=blendZone/2 (halfway) -> chance=0.5, distToEdge=blendZone (interior) -> chance=0.0
                blendChance = 1.0f - (distToEdge / worldGen.blendZone);
            }

            // If we are in the stone layer and in the desert, use the correct blocks
            if (y > stoneStart && biomeNoise[x] > worldGen.minDesertThreshold && biomeNoise[x] < worldGen.maxDesertThreshold)
            {
                // Stone can generate near biome edges
                if (getRandomChance(rng, blendChance))
                    b.type = Block::stone;
                else if (getRandomChance(rng, 0.5f))
                    b.type = Block::sand;
                else
                    b.type = Block::sandStone;

                // Rubies can generate deep in the stone layer
                if (y > worldGen.rubyThreshold)
                {
                    if (getRandomChance(rng, worldGen.rubyChance))
                        b.type = Block::sandRuby;
                    // Copper still has a chance to generate
                    else if (getRandomChance(rng, worldGen.copperChance))
                        b.type = Block::copper;
                    // Other ores can generate near biome edges
                    if (getRandomChance(rng, blendChance))
                    {
                        if (getRandomChance(rng, worldGen.goldChance))
                            b.type = Block::gold;
                        else if (getRandomChance(rng, worldGen.ironChance))
                            b.type = Block::iron;
                    }
                }
                // Not deep enough for rubies, but copper and other ores could still generate
                else if (y > worldGen.oreThreshold)
                {
                    if (getRandomChance(rng, worldGen.copperChance))
                        b.type = Block::copper;
                    else if (getRandomChance(rng, blendChance))
                    {
                        if (getRandomChance(rng, worldGen.goldChance))
                            b.type = Block::gold;
                        else if (getRandomChance(rng, worldGen.ironChance))
                            b.type = Block::iron;
                    }
                }
            }

            // If we are higher up in the desert, sand generates instead of dirt and grass
            else if (y >= dirtStart && biomeNoise[x] > worldGen.minDesertThreshold && biomeNoise[x] < worldGen.maxDesertThreshold)
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

#pragma region generate_caves
            // Band threshold: cave appears only when the *blended* noise lands in the
            // cave band. AND-ing two separate band checks would give intersection
            // (both noises agree); lerp-then-threshold gives a smooth morph between
            // two cave styles across regions.
            if (worldGen.generateCaves)
            {
                bool generateCave = (
                    getFinalCaveNoise(x, y) < worldGen.maxCaveThreshold && getFinalCaveNoise(x, y) > worldGen.minCaveThreshold
                    );

                // Prevent caves from opening up to the void / edge of the map
                if (y == HEIGHT - 1 || x == 0 || x == WIDTH - 1) {}
                // Cave generation
                else if (generateCave)
                {
                    b.type = Block::air;
                    gameMap.getBlockUnsafe(x, y) = b;

                    // The background block shouldn't be air in caves, but the foreground block should be air
                    Block background;
                    background.randIndex = getRandomInt(rng, 0, 3);
                    // If we are in the stone layer in the desert, use the correct background blocks
                    if (y > stoneStart && biomeNoise[x] > worldGen.minDesertThreshold && biomeNoise[x] < worldGen.maxDesertThreshold)
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
            }
#pragma endregion
        }
    }
    
#pragma region spawn_worms
    int numWorms = getRandomInt(rng, worldGen.minNumWorms, worldGen.maxNumWorms);
    worldGen.curNumWorms = numWorms;

    auto spawnWorm = [&](float startX, float startY, int length, int radius, float angle)
        {
            Block b;
            b.type = Block::air;

            for (int step = 0; step < length; step++)
            {
                // Nudge the heading by a small random angle each step.
                // Smaller range = smoother sweeping curves, larger = twistier tunnels.
                // The nudges accumulate over many steps into a gradual wander.
                float turn = getRandomFloat(rng, worldGen.minWormTurnAngle, worldGen.maxWormTurnAngle);
                angle += turn;

                // Convert the heading angle into a unit movement vector via trig.
                // (cos, sin) is the point on the unit circle at this angle, so the
                // vector always has length 1 - the worm moves 1 tile per step
                // regardless of direction.
                float moveX = cosf(angle);
                float moveY = sinf(angle);

                // The worm's position (center of a circle) is stored as a float so it can move at any
                // angle (e.g. (0.87, 0.5) per step at 30°). The map is a grid, so
                // we truncate to ints when we actually need to touch tiles.
                int cx = (int)startX;
                int cy = (int)startY;

                // Carve a disk of radius "radius" around (cx, cy).
                // The two loops walk a (2r+1) x (2r+1) square of offsets around
                // the center; the circle test below skips the corner tiles so
                // what's left is a roughly round disk.
                for (int offsetY = -radius; offsetY <= radius; offsetY++)
                {
                    for (int offsetX = -radius; offsetX <= radius; offsetX++)
                    {
                        // Pythagoras: squared distance from the center.
                        // Skip tiles farther than r from the center (the square's
                        // four corners). Comparing squared values avoids a sqrt.
                        if (offsetX * offsetX + offsetY * offsetY > radius * radius) continue;

                        // Absolute world tile = disk center + offset
                        int tileX = cx + offsetX;
                        int tileY = cy + offsetY;

                        // Stay one tile inside the map edges so worms can't dig
                        // out into the void, and skip tiles that are already air
                        // (no point overwriting air with air, e.g. inside caves).
                        if (tileX > 0 && tileX < WIDTH - 1 && tileY > 0 && tileY < HEIGHT - 1
                            && gameMap.getBlockUnsafe(tileX, tileY).type != Block::air)
                        {
                            gameMap.getBlockUnsafe(tileX, tileY) = b;
                        }
                    }
                }
                // Advance the worm by its heading vector. Because startX/Y are
                // floats, fractional movement (e.g. moveY = 0.296) accumulates
                // across steps instead of getting rounded away each time - this
                // is what lets the worm travel at non-cardinal angles.
                startX += moveX;
                startY += moveY;
            }
        };


    // Worms spawn in a band below the stone layer. If the world is too short
    // to fit that band, skip the worm pass - getRandomInt asserts when min > max.
    int wormMinX = 10;
    int wormMaxX = WIDTH - 10;
    int wormMinY = 375;
    int wormMaxY = HEIGHT - 10;
    if (worldGen.generateWorms && wormMaxX > wormMinX && wormMaxY > wormMinY)
    {
        // Worm pass: each worm wanders through the world carving out a tunnel.
        // Worms have a continuous heading angle (in radians) that drifts slightly
        // every step, so their paths form smooth curves instead of locking onto
        // a fixed direction. At each step the worm stamps a circular disk
        // of air; consecutive disks overlap, producing a continuous tunnel.
        for (int i = 0; i < numWorms; i++)
        {
            float startX = (float)getRandomInt(rng, wormMinX, wormMaxX);
            float startY = (float)getRandomInt(rng, wormMinY, wormMaxY);
            int length = getRandomInt(rng, 50, 500);
            int radius = getRandomInt(rng, worldGen.minWormWidth, worldGen.maxWormWidth);
            float angle = getRandomFloat(rng, 0.0f, 2.0f * 3.14159265f);

            spawnWorm(startX, startY, length, radius, angle);
        }
    }
    else
    {
        worldGen.curNumWorms = 0;
    }
#pragma endregion

    // Free resources
    FastNoiseSIMD::FreeNoiseSet(dirtPlainNoise);
    FastNoiseSIMD::FreeNoiseSet(stonePlainNoise);
    FastNoiseSIMD::FreeNoiseSet(dirtMountainNoise);
    FastNoiseSIMD::FreeNoiseSet(stoneMountainNoise);
    FastNoiseSIMD::FreeNoiseSet(biomeNoise);
    FastNoiseSIMD::FreeNoiseSet(caveNoise1);
    FastNoiseSIMD::FreeNoiseSet(caveNoise2);
    FastNoiseSIMD::FreeNoiseSet(caveSelectorNoise);
}