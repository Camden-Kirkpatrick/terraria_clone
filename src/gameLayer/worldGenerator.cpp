#include "worldGenerator.hpp"
#include "randomStuff.hpp"
#include <FastNoiseSIMD.h>
#include <memory>

WorldGen worldGen;
int worldWidth = DEFAULT_WORLD_WIDTH;
int worldHeight = DEFAULT_WORLD_HEIGHT;
int seed = DEFAULT_SEED;

std::vector<float> savedBiomeNoise;

void resetWorldGen()
{
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

    worldGen.terrainBlendZone = 0.075f;

    // Plain settings
    // Noise generation settings
    worldGen.plainThreshold = 0.5f;
    worldGen.dirtPlainOctaves = 1;
    worldGen.dirtPlainFrequency = 0.0035f;
    worldGen.stonePlainOctaves = 4;
    worldGen.stonePlainFrequency = 0.0075f;
    // Terrain generation settings
    worldGen.minDirtPlainThickness = 1;
    worldGen.maxDirtPlainThickness = 10;
    worldGen.minStonePlainStart = 330;
    worldGen.maxStonePlainStart = 340;

    // Biome settings
    worldGen.terrainOctaves = 1;
    worldGen.terrainFrequency = 0.002f;

    // Desert settings
    // When the biome noise is in this range, deserts will generate
    worldGen.minDesertThreshold = 0.0f;
    worldGen.maxDesertThreshold = 0.4f;
    worldGen.desertOctaves = 1;
    worldGen.desertFrequency = 0.0005;
    // This is the width (in noise units) of the band near each boundary where blending happens
    // With 0.015, only columns whose noise is within 0.015 of a boundary will receive any grassy-biome blocks
    worldGen.desertBlendZone = 0.03f;

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
    worldGen.minWormLength = 50;
    worldGen.maxWormLength = 500;
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

    std::ranlux24_base rng;

#pragma region generate_noise
    // Noise generators for different layers, biomes, and caves
    std::unique_ptr<FastNoiseSIMD> dirtNoiseGenerator(FastNoiseSIMD::NewFastNoiseSIMD());
    std::unique_ptr<FastNoiseSIMD> stoneNoiseGenerator(FastNoiseSIMD::NewFastNoiseSIMD());
    std::unique_ptr<FastNoiseSIMD> terrainNoiseGenerator(FastNoiseSIMD::NewFastNoiseSIMD());
    std::unique_ptr<FastNoiseSIMD> desertNoiseGenerator(FastNoiseSIMD::NewFastNoiseSIMD());
    std::unique_ptr<FastNoiseSIMD> caveNoiseGenerator(FastNoiseSIMD::NewFastNoiseSIMD());

    // Each generator gets a unique seed so their shapes don't match
    dirtNoiseGenerator->SetSeed(seed++);
    stoneNoiseGenerator->SetSeed(seed++);
    terrainNoiseGenerator->SetSeed(seed++);
    desertNoiseGenerator->SetSeed(seed++);
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


    // Noise for switching between plains and mountains
    terrainNoiseGenerator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    terrainNoiseGenerator->SetFractalOctaves(worldGen.terrainOctaves);
    // Lower frequency = larger regions, slower transitions between plains and mountains
    terrainNoiseGenerator->SetFrequency(worldGen.terrainFrequency);

    float* terrainNoise = FastNoiseSIMD::GetEmptySet(WIDTH);

    terrainNoiseGenerator->FillNoiseSet(terrainNoise, 0, 0, 0, WIDTH, 1, 1);


    // Noise for deserts
    desertNoiseGenerator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    desertNoiseGenerator->SetFractalOctaves(worldGen.desertOctaves);
    desertNoiseGenerator->SetFrequency(worldGen.desertFrequency);

    float* desertNoise = FastNoiseSIMD::GetEmptySet(WIDTH);

    desertNoiseGenerator->FillNoiseSet(desertNoise, 0, 0, 0, WIDTH, 1, 1);


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

        terrainNoise[i] = (terrainNoise[i] + 1) / 2;

        desertNoise[i] = (desertNoise[i] + 1) / 2;
    }
    for (int i = 0; i < WIDTH * HEIGHT; i++)
    {
        caveNoise1[i] = (caveNoise1[i] + 1) / 2;
        caveNoise2[i] = (caveNoise2[i] + 1) / 2;
        caveSelectorNoise[i] = (caveSelectorNoise[i] + 1) / 2;
    }

    // Used for displaying the current type of terrain (plains/mountains)
    //savedBiomeNoise.assign(terrainNoise, terrainNoise + WIDTH);

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
        rng.seed(seed + x);

#pragma region linerar_interpolation
        // For this column, compute representative stone-start heights for BOTH biomes.
        // Each one is a lerp from the biome's [min, max] range driven by its own per-column
        // noise array. We need both values regardless of which biome this column ends up in
        // because the blend-zone branch below needs to interpolate between them.
        int stonePlainStart = lerp(worldGen.minStonePlainStart, worldGen.maxStonePlainStart, stonePlainNoise[x]);
        int stoneMountainStart = lerp(worldGen.minStoneMountainStart, worldGen.maxStoneMountainStart, stoneMountainNoise[x]);
        // Same idea for dirt thickness: compute a value for each biome up front, then pick
        // or blend below.
        int dirtPlainThickness = lerp(worldGen.minDirtPlainThickness, worldGen.maxDirtPlainThickness, dirtPlainNoise[x]);
        int dirtMountainThickness = lerp(worldGen.minDirtMountainThickness, worldGen.maxDirtMountainThickness, dirtMountainNoise[x]);

        // Final values that will actually be used to place blocks in this column.
        // Initialized to 0; one of the three branches below will overwrite them.
        int stoneStart = 0;
        int dirtThickness = 0;
        int dirtStart = 0;

        // Three-way decision based on where terrainNoise[x] sits relative to the plains/mountains boundary:
        //   1. Inside the blend zone   -> smoothly mix plains and mountains
        //   2. Below plainThreshold    -> pure plains
        //   3. Above plainThreshold    -> pure mountains
        // Splitting it this way keeps biomes distinct outside the blend zone (no setting leak)
        // while still producing smooth transitions at boundaries.

        // Case 1: blend zone.
        // Triggers when terrainNoise[x] is within terrainBlendZone of plainThreshold on either side.
        // e.g. plainThreshold=0.5, terrainBlendZone=0.05 -> blend zone is (0.45, 0.55).
        if (terrainNoise[x] > worldGen.plainThreshold - worldGen.terrainBlendZone && terrainNoise[x] < worldGen.plainThreshold + worldGen.terrainBlendZone)
        {
            // distToEdge: how far this column's noise is from the boundary (always positive).
            // 0.0 = exactly on the boundary, terrainBlendZone = at the outer edge of the zone.
            float distToEdge = std::abs(terrainNoise[x] - worldGen.plainThreshold);
            // Normalize to [0, 1]: 0.0 at the boundary, 1.0 at the outer edge.
            // This makes the formulas below independent of how wide the blend zone is.
            float ratio = distToEdge / worldGen.terrainBlendZone;

            // t is the blend weight for lerp(plain, mountain, t):
            //   t = 0 -> pure plains
            //   t = 0.5 -> exact 50/50 mix (used right at the boundary)
            //   t = 1 -> pure mountains
            // We need t to slide smoothly from 0 (outer plains edge) -> 0.5 (boundary) -> 1 (outer mountains edge)
            // as terrainNoise[x] walks across the blend zone.
            //
            // ratio is symmetric around the boundary (same magnitude on both sides), so it can't
            // tell t which direction to lean. The terrainNoise < plainThreshold check below picks
            // the correct formula for each side.
            float t;

            if (terrainNoise[x] < worldGen.plainThreshold)
                // Plains side: ratio=0 at boundary -> t=0.5; ratio=1 at outer edge -> t=0.
                t = 0.5f - 0.5f * ratio;
            else
                // Mountains side: ratio=0 at boundary -> t=0.5; ratio=1 at outer edge -> t=1.
                t = 0.5f + 0.5f * ratio;

            // Mix this column's plains and mountains values using the computed blend weight.
            stoneStart = lerp(stonePlainStart, stoneMountainStart, t);
            dirtThickness = lerp(dirtPlainThickness, dirtMountainThickness, t);
        }
        // Case 2: pure plains.
        // terrainNoise is on the plains side AND outside the blend zone.
        // Use plain settings only, with no mountain contamination, so plain settings are
        // fully isolated to plain columns.
        else if (terrainNoise[x] < worldGen.plainThreshold)
        {
            stoneStart = lerp(worldGen.minStonePlainStart, worldGen.maxStonePlainStart, stonePlainNoise[x]);
            dirtThickness = lerp(worldGen.minDirtPlainThickness, worldGen.maxDirtPlainThickness, dirtPlainNoise[x]);
        }
        // Case 3: pure mountains.
        // terrainNoise is on the mountains side AND outside the blend zone.
        // Use mountain settings only, mirror of the plains branch.
        else
        {
            stoneStart = lerp(worldGen.minStoneMountainStart, worldGen.maxStoneMountainStart, stoneMountainNoise[x]);
            dirtThickness = lerp(worldGen.minDirtMountainThickness, worldGen.maxDirtMountainThickness, dirtMountainNoise[x]);
        }

        // Dirt sits on top of stone. Smaller y = higher up in the world, so subtracting
        // dirtThickness from stoneStart gives the y of the highest dirt block (the surface).
        dirtStart = stoneStart - dirtThickness;
#pragma endregion

        // Set the block type based on the current depth
        for (int y = 0; y < HEIGHT; y++)
        {
            Block b;

#pragma region grasslands_biome
            // When y is deeper than the stone surface, stone can generate
            if (y > stoneStart && (desertNoise[x] <= worldGen.minDesertThreshold || desertNoise[x] >= worldGen.maxDesertThreshold))
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
            else if (y > dirtStart && (desertNoise[x] <= worldGen.minDesertThreshold || desertNoise[x] >= worldGen.maxDesertThreshold))
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
            else if (y == dirtStart && (desertNoise[x] <= worldGen.minDesertThreshold || desertNoise[x] >= worldGen.maxDesertThreshold))
                b.type = Block::grassBlock;
#pragma endregion

#pragma region desert_biome
            float distToEdge = 0.0f;
            float blendChance = 0.0f;

            if (desertNoise[x] > worldGen.minDesertThreshold && desertNoise[x] < worldGen.maxDesertThreshold)
            {
                // How close are we to the nearest edge of the desert
                distToEdge = std::min(desertNoise[x] - worldGen.minDesertThreshold, worldGen.maxDesertThreshold - desertNoise[x]);
                // Probability of placing grassy biome blocks instead of desert blocks.
                // High near the desert boundary, zero in the interior.
                // e.g. distToEdge=0.000 (boundary) -> chance=1.0, distToEdge=biomeBlendZone/2 (halfway) -> chance=0.5, distToEdge=biomeBlendZone (interior) -> chance=0.0
                blendChance = 1.0f - (distToEdge / worldGen.desertBlendZone);
            }

            // If we are in the stone layer and in the desert, use the correct blocks
            if (y > stoneStart && desertNoise[x] > worldGen.minDesertThreshold && desertNoise[x] < worldGen.maxDesertThreshold)
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
            else if (y >= dirtStart && desertNoise[x] > worldGen.minDesertThreshold && desertNoise[x] < worldGen.maxDesertThreshold)
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
                    if (y > stoneStart && desertNoise[x] > worldGen.minDesertThreshold && desertNoise[x] < worldGen.maxDesertThreshold)
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
    rng.seed(seed + 2112);

    // Depending on the world size, there can be more or less tunnels
    worldGen.minNumWorms = (int)(worldWidth / MIN_WORM_DIVISOR);
    worldGen.maxNumWorms = (int)(worldWidth / MAX_WORM_DIVISOR);

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
    int wormMinY = 350;
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
            int length = getRandomInt(rng, worldGen.minWormLength, worldGen.maxWormLength);
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
    FastNoiseSIMD::FreeNoiseSet(terrainNoise);
    FastNoiseSIMD::FreeNoiseSet(caveNoise1);
    FastNoiseSIMD::FreeNoiseSet(caveNoise2);
    FastNoiseSIMD::FreeNoiseSet(caveSelectorNoise);
}