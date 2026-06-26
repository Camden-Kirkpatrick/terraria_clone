#include "worldGenerator.hpp"
#include "randomStuff.hpp"
#include "structure.hpp"
#include "saveMap.hpp"
#include <FastNoiseSIMD.h>
#include <memory>
#include <iostream>

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

    worldGen.terrainBlendZone = 0.045f;

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

    // Terrain settings
    worldGen.terrainOctaves = 1;
    worldGen.terrainFrequency = 0.002f;

    // Biome settings
    worldGen.biomeOctaves = 1;
    worldGen.biomeFrequency = 0.00033;
    // This is the width (in noise units) of the band near each boundary where blending happens
    // With 0.015, only columns whose noise is within 0.015 of a boundary will receive any grassy-biome blocks
    worldGen.biomeBlendZone = 0.03f;
    // Desert settings
    worldGen.minDesertThreshold = 0.0f;
    worldGen.maxDesertThreshold = 0.4f;
    // Tundra settings
    worldGen.minTundraThreshold = 0.6f;
    worldGen.maxTundraThreshold = 0.8f;

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

    // Tree settings
    worldGen.generateTrees = true;
    worldGen.treeSpawnChance = 0.04f;
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

// Inverse of lerp: given a value in [a, b], returns its position in [0, 1].
float invLerp(float a, float b, float v)
{
    return (v - a) / (b - a);
}

// Converts a column's terrain noise into a plains<->mountains blend weight in [0, 1]:
//   0.0 = pure plains, 1.0 = pure mountains, values between = blend.
// Outside the blend zone it snaps to 0 or 1 so biomes stay distinct; inside the
// zone it ramps smoothly across so the transition has no hard seam.
// The result is meant to feed lerp(plainsValue, mountainsValue, t).
float terrainBlend(float terrainNoise)
{
    // Edges of the blend zone
    float lo = worldGen.plainThreshold - worldGen.terrainBlendZone;
    float hi = worldGen.plainThreshold + worldGen.terrainBlendZone;

    // Plains
    if (terrainNoise <= lo)
        return 0.0f;
    // Mountains
    if (terrainNoise >= hi)
        return 1.0f;

    return invLerp(lo, hi, terrainNoise);
}

void generateWorld(GameMap& gameMap, const int WIDTH, const int HEIGHT, int seed, bool resetWrldGen)
{
    if (resetWrldGen)
        resetWorldGen();

    gameMap.create(WIDTH, HEIGHT);

    std::ranlux24_base rng;

    // Tree textures
    Structure tree1;
    loadBlockDataFromFile(
        tree1.structureBlocks,
        tree1.structureWallBlocks,
        tree1.w,
        tree1.h,
        RESOURCES_PATH "structures/tree1.bin"
    );

    Structure tree2;
    loadBlockDataFromFile(
        tree2.structureBlocks,
        tree2.structureWallBlocks,
        tree2.w,
        tree2.h,
        RESOURCES_PATH "structures/tree2.bin"
    );

    Structure tree3;
    loadBlockDataFromFile(
        tree3.structureBlocks,
        tree3.structureWallBlocks,
        tree3.w,
        tree3.h,
        RESOURCES_PATH "structures/tree3.bin"
    );

    Structure trees[3] = { tree1, tree2, tree3 };
#pragma region generate_noise
    // Noise generators for different layers, biomes, and caves
    std::unique_ptr<FastNoiseSIMD> dirtNoiseGenerator(FastNoiseSIMD::NewFastNoiseSIMD());
    std::unique_ptr<FastNoiseSIMD> stoneNoiseGenerator(FastNoiseSIMD::NewFastNoiseSIMD());
    std::unique_ptr<FastNoiseSIMD> terrainNoiseGenerator(FastNoiseSIMD::NewFastNoiseSIMD());
    std::unique_ptr<FastNoiseSIMD> biomeNoiseGenerator(FastNoiseSIMD::NewFastNoiseSIMD());
    std::unique_ptr<FastNoiseSIMD> caveNoiseGenerator(FastNoiseSIMD::NewFastNoiseSIMD());

    // Each generator gets a unique seed so their shapes don't match
    dirtNoiseGenerator->SetSeed(seed++);
    stoneNoiseGenerator->SetSeed(seed++);
    terrainNoiseGenerator->SetSeed(seed++);
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


    // Noise for switching between plains and mountains
    terrainNoiseGenerator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    terrainNoiseGenerator->SetFractalOctaves(worldGen.terrainOctaves);
    // Lower frequency = larger regions, slower transitions between plains and mountains
    terrainNoiseGenerator->SetFrequency(worldGen.terrainFrequency);

    float* terrainNoise = FastNoiseSIMD::GetEmptySet(WIDTH);

    terrainNoiseGenerator->FillNoiseSet(terrainNoise, 0, 0, 0, WIDTH, 1, 1);


    // Noise for biomes
    biomeNoiseGenerator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    biomeNoiseGenerator->SetFractalOctaves(worldGen.biomeOctaves);
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

        terrainNoise[i] = (terrainNoise[i] + 1) / 2;

        biomeNoise[i] = (biomeNoise[i] + 1) / 2;
    }
    for (int i = 0; i < WIDTH * HEIGHT; i++)
    {
        caveNoise1[i] = (caveNoise1[i] + 1) / 2;
        caveNoise2[i] = (caveNoise2[i] + 1) / 2;
        caveSelectorNoise[i] = (caveSelectorNoise[i] + 1) / 2;
    }

    // Used for displaying the current type of terrain (plains/mountains)
    savedBiomeNoise.assign(terrainNoise, terrainNoise + WIDTH);

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
//    for (int x = 0; x < WIDTH; x++)
//    {
//        rng.seed(seed + x);
//
//#pragma region linerar_interpolation
//        // For this column, compute representative stone-start heights for BOTH biomes.
//        // Each one is a lerp from the biome's [min, max] range driven by its own per-column
//        // noise array. We need both values regardless of which biome this column ends up in
//        // because the blend-zone branch below needs to interpolate between them.
//        int stonePlainStart = lerp(worldGen.minStonePlainStart, worldGen.maxStonePlainStart, stonePlainNoise[x]);
//        int stoneMountainStart = lerp(worldGen.minStoneMountainStart, worldGen.maxStoneMountainStart, stoneMountainNoise[x]);
//        // Same idea for dirt thickness: compute a value for each biome up front, then pick
//        // or blend below.
//        int dirtPlainThickness = lerp(worldGen.minDirtPlainThickness, worldGen.maxDirtPlainThickness, dirtPlainNoise[x]);
//        int dirtMountainThickness = lerp(worldGen.minDirtMountainThickness, worldGen.maxDirtMountainThickness, dirtMountainNoise[x]);
//
//        // Final values that will actually be used to place blocks in this column.
//        // Initialized to 0; one of the three branches below will overwrite them.
//        int stoneStart = 0;
//        int dirtThickness = 0;
//        int dirtStart = 0;
//
//        // Three-way decision based on where terrainNoise[x] sits relative to the plains/mountains boundary:
//        //   1. Inside the blend zone   -> smoothly mix plains and mountains
//        //   2. Below plainThreshold    -> pure plains
//        //   3. Above plainThreshold    -> pure mountains
//        // Splitting it this way keeps biomes distinct outside the blend zone (no setting leak)
//        // while still producing smooth transitions at boundaries.
//
//        // Case 1: blend zone.
//        // Triggers when terrainNoise[x] is within terrainBlendZone of plainThreshold on either side.
//        // e.g. plainThreshold=0.5, terrainBlendZone=0.05 -> blend zone is (0.45, 0.55).
//        if (terrainNoise[x] > worldGen.plainThreshold - worldGen.terrainBlendZone && terrainNoise[x] < worldGen.plainThreshold + worldGen.terrainBlendZone)
//        {
//            // distToEdge: how far this column's noise is from the boundary (always positive).
//            // 0.0 = exactly on the boundary, terrainBlendZone = at the outer edge of the zone.
//            float distToEdge = std::abs(terrainNoise[x] - worldGen.plainThreshold);
//            // Normalize to [0, 1]: 0.0 at the boundary, 1.0 at the outer edge.
//            // This makes the formulas below independent of how wide the blend zone is.
//            float ratio = distToEdge / worldGen.terrainBlendZone;
//
//            // t is the blend weight for lerp(plain, mountain, t):
//            //   t = 0 -> pure plains
//            //   t = 0.5 -> exact 50/50 mix (used right at the boundary)
//            //   t = 1 -> pure mountains
//            // We need t to slide smoothly from 0 (outer plains edge) -> 0.5 (boundary) -> 1 (outer mountains edge)
//            // as terrainNoise[x] walks across the blend zone.
//            //
//            // ratio is symmetric around the boundary (same magnitude on both sides), so it can't
//            // tell t which direction to lean. The terrainNoise < plainThreshold check below picks
//            // the correct formula for each side.
//            float t;
//
//            if (terrainNoise[x] < worldGen.plainThreshold)
//                // Plains side: ratio=0 at boundary -> t=0.5; ratio=1 at outer edge -> t=0.
//                t = 0.5f - 0.5f * ratio;
//            else
//                // Mountains side: ratio=0 at boundary -> t=0.5; ratio=1 at outer edge -> t=1.
//                t = 0.5f + 0.5f * ratio;
//
//            // Mix this column's plains and mountains values using the computed blend weight.
//            stoneStart = lerp(stonePlainStart, stoneMountainStart, t);
//            dirtThickness = lerp(dirtPlainThickness, dirtMountainThickness, t);
//        }
//        // Case 2: pure plains.
//        // terrainNoise is on the plains side AND outside the blend zone.
//        else if (terrainNoise[x] < worldGen.plainThreshold)
//        {
//            // Extra variation only kicks in for the RARE low tail of terrainNoise.
//            // Simplex noise clusters around 0.5, so values below variationStart are
//            // uncommon - those columns get the dramatic treatment, the rest stay flat.
//            const float variationStart = 0.33f;   // below this, variation ramps in
//            const float maxStoneOffset = 12.0f;   // most we shift the stone range by
//            const float maxDirtOffset = 12.0f;   // most we widen the dirt range by
//
//            // depth = "how deep into the rare tail is this column?", as a 0..1 fraction.
//            //   terrainNoise == variationStart -> depth 0 (no extra variation)
//            //   terrainNoise == 0              -> depth 1 (max variation)
//            // Dividing by variationStart rescales the [0, variationStart] range to [0, 1],
//            // so the math below doesn't care how wide the tail happens to be. Because depth
//            // changes continuously, the variation eases in with no sudden steps.
//            float depth = 0.0f;
//            if (terrainNoise[x] < variationStart)
//                depth = (variationStart - terrainNoise[x]) / variationStart;
//
//            // Scale the offsets by depth: 0 for common columns, up to the max for the rarest.
//            float stoneOffset = depth * maxStoneOffset;
//            float dirtOffset = depth * maxDirtOffset;
//
//            // Allow hilly plains: subtract from the MIN so stoneStart can drop below its
//            // normal floor. Smaller stoneStart = higher surface = taller terrain. This only
//            // extends the tall end of the range, so rare columns can spike up into hills
//            // while neighbours stay low -> steeper, hillier plains. Subtracting from the min
//            // (not adding to the max) is also why the surface goes UP instead of down.
//            stoneStart = lerp(worldGen.minStonePlainStart - stoneOffset, worldGen.maxStonePlainStart, stonePlainNoise[x]);
//
//            // Allow the dirt layer to be thicker: widen only the MAX, never the min, so
//            // dirtThickness can grow but can never go negative (a negative thickness would
//            // flip dirtStart below stoneStart and wipe out the grass surface).
//            dirtThickness = lerp(worldGen.minDirtPlainThickness, worldGen.maxDirtPlainThickness + dirtOffset, dirtPlainNoise[x]);
//        }
//        // Case 3: pure mountains.
//        // terrainNoise is on the mountains side AND outside the blend zone.
//        // Mirror of the plains branch, but variation comes from the rare HIGH tail.
//        else
//        {
//            // Mountains use the high tail of terrainNoise, and bigger offsets than plains
//            // for more dramatic peaks.
//            const float variationStart = 0.66f;   // above this, variation ramps in
//            const float maxStoneOffset = 30.0f;
//            const float maxDirtOffset = 30.0f;
//
//            // Same depth idea, flipped to measure distance into the HIGH tail:
//            //   terrainNoise == variationStart -> depth 0
//            //   terrainNoise == 1              -> depth 1
//            // Dividing by (1 - variationStart) rescales [variationStart, 1] to [0, 1].
//            float depth = 0.0f;
//            if (terrainNoise[x] > variationStart)
//                depth = (terrainNoise[x] - variationStart) / (1.0f - variationStart);
//
//            float stoneOffset = depth * maxStoneOffset;
//            float dirtOffset = depth * maxDirtOffset;
//
//            // Allow taller mountains: subtract from the MIN so stoneStart can drop lower
//            // (smaller y), pushing the surface higher. Same direction as plains, just a
//            // larger offset for bigger peaks.
//            stoneStart = lerp(worldGen.minStoneMountainStart - stoneOffset, worldGen.maxStoneMountainStart, stoneMountainNoise[x]);
//
//            // Allow the dirt layer to be thicker: widen only the max (keeps thickness >= 1).
//            dirtThickness = lerp(worldGen.minDirtMountainThickness, worldGen.maxDirtMountainThickness + dirtOffset, dirtMountainNoise[x]);
//        }
//
//        // Dirt sits on top of stone. Smaller y = higher up in the world, so subtracting
//        // dirtThickness from stoneStart gives the y of the highest dirt block (the surface).
//        dirtStart = stoneStart - dirtThickness;
//#pragma endregion
//
//        // Set the block type based on the current depth
//        for (int y = 0; y < HEIGHT; y++)
//        {
//            Block b;
//
//#pragma region grasslands_biome
//            // When y is deeper than the stone surface, stone can generate
//            if (y > stoneStart && (desertNoise[x] <= worldGen.minDesertThreshold || desertNoise[x] >= worldGen.maxDesertThreshold))
//            {
//                b.type = Block::stone;
//                // Gold can generate further down in the stone layer
//                if (y > worldGen.oreThreshold)
//                {
//                    // worldGen.goldChance chance for gold to generate instead of stone
//                    if (getRandomChance(rng, worldGen.goldChance))
//                        b.type = Block::gold;
//                    // If gold doesn't generate, iron has a chance to
//                    else if (getRandomChance(rng, worldGen.ironChance))
//                        b.type = Block::iron;
//                }
//            }
//
//            // When y is above the dirtHeight threshold, dirt can generate
//            else if (y > dirtStart && (desertNoise[x] <= worldGen.minDesertThreshold || desertNoise[x] >= worldGen.maxDesertThreshold))
//            {
//                b.type = Block::dirt;
//                // Clay can generate further down in the dirt layer
//                if (y > worldGen.clayThreshold)
//                {
//                    // worldGen.clayChance chance for clay to generate instead of dirt
//                    if (getRandomChance(rng, worldGen.clayChance))
//                        b.type = Block::clay;
//                }
//            }
//
//            // When y is exactly equal to the dirtHeight threshold, grass generates
//            else if (y == dirtStart && (desertNoise[x] <= worldGen.minDesertThreshold || desertNoise[x] >= worldGen.maxDesertThreshold))
//                b.type = Block::grassBlock;
//#pragma endregion
//
//#pragma region desert_biome
//            float distToEdge = 0.0f;
//            float blendChance = 0.0f;
//
//            if (desertNoise[x] > worldGen.minDesertThreshold && desertNoise[x] < worldGen.maxDesertThreshold)
//            {
//                // How close are we to the nearest edge of the desert
//                distToEdge = std::min(desertNoise[x] - worldGen.minDesertThreshold, worldGen.maxDesertThreshold - desertNoise[x]);
//                // Probability of placing grassy biome blocks instead of desert blocks.
//                // High near the desert boundary, zero in the interior.
//                // e.g. distToEdge=0.000 (boundary) -> chance=1.0, distToEdge=biomeBlendZone/2 (halfway) -> chance=0.5, distToEdge=biomeBlendZone (interior) -> chance=0.0
//                blendChance = 1.0f - (distToEdge / worldGen.desertBlendZone);
//            }
//
//            // If we are in the stone layer and in the desert, use the correct blocks
//            if (y > stoneStart && desertNoise[x] > worldGen.minDesertThreshold && desertNoise[x] < worldGen.maxDesertThreshold)
//            {
//                // Stone can generate near biome edges
//                if (getRandomChance(rng, blendChance))
//                    b.type = Block::stone;
//                else if (getRandomChance(rng, 0.5f))
//                    b.type = Block::sand;
//                else
//                    b.type = Block::sandStone;
//
//                // Rubies can generate deep in the stone layer
//                if (y > worldGen.rubyThreshold)
//                {
//                    if (getRandomChance(rng, worldGen.rubyChance))
//                        b.type = Block::sandRuby;
//                    // Copper still has a chance to generate
//                    else if (getRandomChance(rng, worldGen.copperChance))
//                        b.type = Block::copper;
//                    // Other ores can generate near biome edges
//                    if (getRandomChance(rng, blendChance))
//                    {
//                        if (getRandomChance(rng, worldGen.goldChance))
//                            b.type = Block::gold;
//                        else if (getRandomChance(rng, worldGen.ironChance))
//                            b.type = Block::iron;
//                    }
//                }
//                // Not deep enough for rubies, but copper and other ores could still generate
//                else if (y > worldGen.oreThreshold)
//                {
//                    if (getRandomChance(rng, worldGen.copperChance))
//                        b.type = Block::copper;
//                    else if (getRandomChance(rng, blendChance))
//                    {
//                        if (getRandomChance(rng, worldGen.goldChance))
//                            b.type = Block::gold;
//                        else if (getRandomChance(rng, worldGen.ironChance))
//                            b.type = Block::iron;
//                    }
//                }
//            }
//
//            // If we are higher up in the desert, sand generates instead of dirt and grass
//            else if (y >= dirtStart && desertNoise[x] > worldGen.minDesertThreshold && desertNoise[x] < worldGen.maxDesertThreshold)
//            {
//                b.type = Block::sand;
//
//                // Grass dirt, and clay blocks can generate near biome edges
//                if (getRandomChance(rng, blendChance))
//                {
//                    if (y == dirtStart)
//                        b.type = Block::grassBlock;
//                    else
//                    {
//                        b.type = Block::dirt;
//
//                        if (y > worldGen.clayThreshold)
//                        {
//                            if (getRandomChance(rng, worldGen.clayChance))
//                                b.type = Block::clay;
//                        }
//                    }
//                        
//                }
//            }
//#pragma endregion
//
//
//            // Each block will use one of 4 random texture variations
//            b.randIndex = getRandomInt(rng, 0, 3);
//
//            // Store the block in the map
//            gameMap.getBlockUnsafe(x, y) = b;
//
//            // Use the correct background based on the block placed
//            gameMap.getWallBlockUnsafe(x, y) = b;
//
//#pragma region generate_caves
//            // Band threshold: cave appears only when the *blended* noise lands in the
//            // cave band. AND-ing two separate band checks would give intersection
//            // (both noises agree); lerp-then-threshold gives a smooth morph between
//            // two cave styles across regions.
//            if (worldGen.generateCaves)
//            {
//                bool generateCave = (
//                    getFinalCaveNoise(x, y) < worldGen.maxCaveThreshold && getFinalCaveNoise(x, y) > worldGen.minCaveThreshold
//                );
//
//                // Prevent caves from opening up to the void / edge of the map
//                if (y == HEIGHT - 1 || x == 0 || x == WIDTH - 1) {}
//                // Cave generation
//                else if (generateCave)
//                {
//                    b.type = Block::air;
//                    gameMap.getBlockUnsafe(x, y) = b;
//
//                    // The background block shouldn't be air in caves, but the foreground block should be air
//                    Block background;
//                    background.randIndex = getRandomInt(rng, 0, 3);
//                    // If we are in the stone layer in the desert, use the correct background blocks
//                    if (y > stoneStart && desertNoise[x] > worldGen.minDesertThreshold && desertNoise[x] < worldGen.maxDesertThreshold)
//                    {
//                        if (getRandomChance(rng, blendChance))
//                            background.type = Block::stone;
//                        else if (getRandomChance(rng, 0.5f))
//                            background.type = Block::sand;
//                        else
//                            background.type = Block::sandStone;
//
//                        gameMap.getWallBlockUnsafe(x, y) = background;
//                    }
//                    // If we are in the stone layer in the grasslands, use the correct background block
//                    else if (y > stoneStart)
//                    {
//                        background.type = Block::stone;
//                        gameMap.getWallBlockUnsafe(x, y) = background;
//                    }
//                }
//            }
//#pragma endregion
//        }
//    }
//    
//#pragma region spawn_worms
//    rng.seed(seed + 2112);
//
//    // Depending on the world size, there can be more or less tunnels
//    worldGen.minNumWorms = (int)(worldWidth / MIN_WORM_DIVISOR);
//    worldGen.maxNumWorms = (int)(worldWidth / MAX_WORM_DIVISOR);
//
//    int numWorms = getRandomInt(rng, worldGen.minNumWorms, worldGen.maxNumWorms);
//    worldGen.curNumWorms = numWorms;
//
//    auto spawnWorm = [&](float startX, float startY, int length, int radius, float angle)
//        {
//            Block b;
//            b.type = Block::air;
//
//            for (int step = 0; step < length; step++)
//            {
//                // Nudge the heading by a small random angle each step.
//                // Smaller range = smoother sweeping curves, larger = twistier tunnels.
//                // The nudges accumulate over many steps into a gradual wander.
//                float turn = getRandomFloat(rng, worldGen.minWormTurnAngle, worldGen.maxWormTurnAngle);
//                angle += turn;
//
//                // Convert the heading angle into a unit movement vector via trig.
//                // (cos, sin) is the point on the unit circle at this angle, so the
//                // vector always has length 1 - the worm moves 1 tile per step
//                // regardless of direction.
//                float moveX = cosf(angle);
//                float moveY = sinf(angle);
//
//                // The worm's position (center of a circle) is stored as a float so it can move at any
//                // angle (e.g. (0.87, 0.5) per step at 30°). The map is a grid, so
//                // we truncate to ints when we actually need to touch tiles.
//                int cx = (int)startX;
//                int cy = (int)startY;
//
//                // Carve a disk of radius "radius" around (cx, cy).
//                // The two loops walk a (2r+1) x (2r+1) square of offsets around
//                // the center; the circle test below skips the corner tiles so
//                // what's left is a roughly round disk.
//                for (int offsetY = -radius; offsetY <= radius; offsetY++)
//                {
//                    for (int offsetX = -radius; offsetX <= radius; offsetX++)
//                    {
//                        // Pythagoras: squared distance from the center.
//                        // Skip tiles farther than r from the center (the square's
//                        // four corners). Comparing squared values avoids a sqrt.
//                        if (offsetX * offsetX + offsetY * offsetY > radius * radius) continue;
//
//                        // Absolute world tile = disk center + offset
//                        int tileX = cx + offsetX;
//                        int tileY = cy + offsetY;
//
//                        // Stay one tile inside the map edges so worms can't dig
//                        // out into the void, and skip tiles that are already air
//                        // (no point overwriting air with air, e.g. inside caves).
//                        if (tileX > 0 && tileX < WIDTH - 1 && tileY > 0 && tileY < HEIGHT - 1
//                            && gameMap.getBlockUnsafe(tileX, tileY).type != Block::air)
//                        {
//                            gameMap.getBlockUnsafe(tileX, tileY) = b;
//                        }
//                    }
//                }
//                // Advance the worm by its heading vector. Because startX/Y are
//                // floats, fractional movement (e.g. moveY = 0.296) accumulates
//                // across steps instead of getting rounded away each time - this
//                // is what lets the worm travel at non-cardinal angles.
//                startX += moveX;
//                startY += moveY;
//            }
//        };
//
//
//    // Worms spawn in a band below the stone layer. If the world is too short
//    // to fit that band, skip the worm pass - getRandomInt asserts when min > max.
//    int wormMinX = 10;
//    int wormMaxX = WIDTH - 10;
//    int wormMinY = 350;
//    int wormMaxY = HEIGHT - 10;
//    if (worldGen.generateWorms && wormMaxX > wormMinX && wormMaxY > wormMinY)
//    {
//        // Worm pass: each worm wanders through the world carving out a tunnel.
//        // Worms have a continuous heading angle (in radians) that drifts slightly
//        // every step, so their paths form smooth curves instead of locking onto
//        // a fixed direction. At each step the worm stamps a circular disk
//        // of air; consecutive disks overlap, producing a continuous tunnel.
//        for (int i = 0; i < numWorms; i++)
//        {
//            float startX = (float)getRandomInt(rng, wormMinX, wormMaxX);
//            float startY = (float)getRandomInt(rng, wormMinY, wormMaxY);
//            int length = getRandomInt(rng, worldGen.minWormLength, worldGen.maxWormLength);
//            int radius = getRandomInt(rng, worldGen.minWormWidth, worldGen.maxWormWidth);
//            float angle = getRandomFloat(rng, 0.0f, 2.0f * 3.14159265f);
//
//            spawnWorm(startX, startY, length, radius, angle);
//        }
//    }
//    else
//    {
//        worldGen.curNumWorms = 0;
//    }
//#pragma endregion
//
//#pragma region generate_trees
//    if (worldGen.generateTrees)
//    {
//        for (int x = 0; x < WIDTH; x++)
//        {
//            if (getRandomChance(rng, worldGen.treeSpawnChance))
//            {
//                Structure tree = trees[getRandomInt(rng, 0, 2)];
//
//                for (int y = 0; y < HEIGHT; y++)
//                {
//                    // Get the current block type
//                    uint16_t type = gameMap.getBlockType(x, y);
//                    // Ignore air blocks
//                    if (type == Block::air)
//                        continue;
//                    // Generate a tree
//                    else if (type == Block::grassBlock)
//                    {
//                        Vector2 spawnPos = { (float)x, (float)y };
//
//                        // Top-left of the tree (tree start)
//                        spawnPos.x -= tree.w / 2;
//                        spawnPos.y -= tree.h;
//
//                        tree.pasteIntoMap(gameMap, spawnPos);
//
//                        // Leave a gap between trees
//                        x += 5;
//
//                        break;
//                    }
//                    // Not a grass block
//                    else
//                        break;
//                }
//            }
//        }
//    }
//#pragma endregion









    // We need this later for the dirt layer, so we can determine where the dirt layer starts
    std::vector<int> stoneLayer(WIDTH, 0);

    auto generateStoneLayer = [&]()
    {
        for (int x = 0; x < WIDTH; x++)
        {
            int stonePlainStart = lerp(worldGen.minStonePlainStart, worldGen.maxStonePlainStart, stonePlainNoise[x]);
            int stoneMountainStart = lerp(worldGen.minStoneMountainStart, worldGen.maxStoneMountainStart, stoneMountainNoise[x]);

            int stoneStart;

            float t = terrainBlend(terrainNoise[x]);

            // Plains
            if (t <= 0.0f)
            {
                const float variationStart = 0.33f;
                const float maxStoneOffset = 12.0f;

                float depth = 0.0f;
                if (terrainNoise[x] < variationStart)
                    depth = (variationStart - terrainNoise[x]) / variationStart;

                float stoneOffset = depth * maxStoneOffset;

                stoneStart = lerp(worldGen.minStonePlainStart - stoneOffset, worldGen.maxStonePlainStart, stonePlainNoise[x]);
            }
            // Mountains
            else if (t >= 1.0f)
            {
                const float variationStart = 0.66f;
                const float maxStoneOffset = 30.0f;

                float depth = 0.0f;
                if (terrainNoise[x] > variationStart)
                    depth = (terrainNoise[x] - variationStart) / (1.0f - variationStart);

                float stoneOffset = depth * maxStoneOffset;

                stoneStart = lerp(worldGen.minStoneMountainStart - stoneOffset, worldGen.maxStoneMountainStart, stoneMountainNoise[x]);
            }
            // Blend zone between plains and mountains
            else
                stoneStart = lerp(stonePlainStart, stoneMountainStart, t);


            // Store the current stoneStart
            stoneLayer[x] = stoneStart;

            for (int y = 0; y < HEIGHT; y++)
            {
                // Ignore, since these will be other blocks (grass, dirt, etc.)
                if (y <= stoneStart)
                    continue;

                Block b;

                b.type = Block::stone;

                // Each block will use a random texture variation.
                b.randIndex = std::rand() % 4;

                // Set the map to use the correct block
                gameMap.getBlockUnsafe(x, y) = b;

                // Use the correct background based on the block placed
                gameMap.getWallBlockUnsafe(x, y) = b;
            }
        }
    };

    std::vector<int> dirtLayer(WIDTH, 0);

    auto generateDirtLayer = [&]()
    {
        for (int x = 0; x < WIDTH; x++)
        {
            int dirtPlainThickness = lerp(worldGen.minDirtPlainThickness, worldGen.maxDirtPlainThickness, dirtPlainNoise[x]);
            int dirtMountainThickness = lerp(worldGen.minDirtMountainThickness, worldGen.maxDirtMountainThickness, dirtMountainNoise[x]);

            int dirtThickness = 0;
            int dirtStart = 0;

            float t = terrainBlend(terrainNoise[x]);

            // Plains
            if (t <= 0.0f)
            {
                const float variationStart = 0.33f;
                const float maxDirtOffset = 12.0f;

                float depth = 0.0f;
                if (terrainNoise[x] < variationStart)
                    depth = (variationStart - terrainNoise[x]) / variationStart;

                float dirtOffset = depth * maxDirtOffset;

                dirtThickness = lerp(worldGen.minDirtPlainThickness, worldGen.maxDirtPlainThickness + dirtOffset, dirtPlainNoise[x]);
            }
            // Mountains
            else if (t >= 1.0f)
            {
                const float variationStart = 0.66f;
                const float maxDirtOffset = 30.0f;

                float depth = 0.0f;
                if (terrainNoise[x] > variationStart)
                    depth = (terrainNoise[x] - variationStart) / (1.0f - variationStart);

                float dirtOffset = depth * maxDirtOffset;

                dirtThickness = lerp(worldGen.minDirtMountainThickness, worldGen.maxDirtMountainThickness + dirtOffset, dirtMountainNoise[x]);
            }
            // Blend zone between plains and mountains
            else
                dirtThickness = lerp(dirtPlainThickness, dirtMountainThickness, t);

            dirtStart = stoneLayer[x] - dirtThickness;

            dirtLayer[x] = dirtStart;

            for (int y = 0; y < HEIGHT; y++)
            {
                // Ignore air
                if (y < dirtStart)
                    continue;
                // Stop when the stone layer is reached
                if (y > stoneLayer[x])
                    break;

                Block b;

                if (y > dirtStart)
                    b.type = Block::dirt;
                else if (y == dirtStart)
                    b.type = Block::grassBlock;

                b.randIndex = std::rand() % 4;

                gameMap.getBlockUnsafe(x, y) = b;

                gameMap.getWallBlockUnsafe(x, y) = b;
            }
        }
    };



    auto generateGrasslands = [&]()
    {
        for (int x = 0; x < WIDTH; x++)
        {
            for (int y = 0; y < HEIGHT; y++)
            {
                Block b;

                // When y is deeper than the stone surface, stone can generate
                if (y > stoneLayer[x])
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
                else if (y > dirtLayer[x])
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
                else if (y == dirtLayer[x])
                    b.type = Block::grassBlock;

                b.randIndex = std::rand() % 4;

                gameMap.getBlockUnsafe(x, y) = b;

                gameMap.getWallBlockUnsafe(x, y) = b;
            }
        }
    };



    auto generateDesert = [&]()
    {
        for (int x = 0; x < WIDTH; x++)
        {
            // Not a desert, so skip this column
            if (!(biomeNoise[x] > worldGen.minDesertThreshold) || !(biomeNoise[x] < worldGen.maxDesertThreshold))
                continue;

            for (int y = 0; y < HEIGHT; y++)
            {
                Block b;

                float distToEdge = 0.0f;
                float blendChance = 0.0f;

                if (biomeNoise[x] > worldGen.minDesertThreshold && biomeNoise[x] < worldGen.maxDesertThreshold)
                {
                    // How close are we to the nearest edge of the desert
                    distToEdge = std::min(biomeNoise[x] - worldGen.minDesertThreshold, worldGen.maxDesertThreshold - biomeNoise[x]);
                    // Probability of placing grassy biome blocks instead of desert blocks.
                    // High near the desert boundary, zero in the interior.
                    // e.g. distToEdge=0.000 (boundary) -> chance=1.0, distToEdge=biomeBlendZone/2 (halfway) -> chance=0.5, distToEdge=biomeBlendZone (interior) -> chance=0.0
                    blendChance = 1.0f - (distToEdge / worldGen.biomeBlendZone);
                }

                // If we are in the stone layer and in the desert, use the correct blocks
                if (y > stoneLayer[x] && biomeNoise[x] > worldGen.minDesertThreshold && biomeNoise[x] < worldGen.maxDesertThreshold)
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
                else if (y >= dirtLayer[x] && biomeNoise[x] > worldGen.minDesertThreshold && biomeNoise[x] < worldGen.maxDesertThreshold)
                {
                    b.type = Block::sand;

                    // Grass dirt, and clay blocks can generate near biome edges
                    if (getRandomChance(rng, blendChance))
                    {
                        if (y == dirtLayer[x])
                            b.type = Block::grassBlock;
                        else
                        {
                            b.type = Block::dirt;

                            if (y > worldGen.clayThreshold)
                            {
                                if (getRandomChance(rng, worldGen.clayChance))
                                    b.type = Block::clay;
                            }
                        }

                    }
                }

                b.randIndex = std::rand() % 4;

                gameMap.getBlockUnsafe(x, y) = b;

                gameMap.getWallBlockUnsafe(x, y) = b;
            }
        }
    };



    auto generateTundra = [&]()
    {
        for (int x = 0; x < WIDTH; x++)
        {
            // Not a desert, so skip this column
            if (!(biomeNoise[x] > worldGen.minTundraThreshold) || !(biomeNoise[x] < worldGen.maxTundraThreshold))
                continue;

            for (int y = 0; y < HEIGHT; y++)
            {
                Block b;

                float distToEdge = 0.0f;
                float blendChance = 0.0f;

                if (biomeNoise[x] > worldGen.minTundraThreshold && biomeNoise[x] < worldGen.maxTundraThreshold)
                {
                    // How close are we to the nearest edge of the desert
                    distToEdge = std::min(biomeNoise[x] - worldGen.minTundraThreshold, worldGen.maxTundraThreshold - biomeNoise[x]);
                    // Probability of placing grassy biome blocks instead of desert blocks.
                    // High near the desert boundary, zero in the interior.
                    // e.g. distToEdge=0.000 (boundary) -> chance=1.0, distToEdge=biomeBlendZone/2 (halfway) -> chance=0.5, distToEdge=biomeBlendZone (interior) -> chance=0.0
                    blendChance = 1.0f - (distToEdge / worldGen.biomeBlendZone);
                }

                // If we are in the stone layer and in the desert, use the correct blocks
                if (y > stoneLayer[x] && biomeNoise[x] > worldGen.minTundraThreshold && biomeNoise[x] < worldGen.maxTundraThreshold)
                {
                    // Stone can generate near biome edges
                    if (getRandomChance(rng, blendChance))
                        b.type = Block::stone;
                    else if (getRandomChance(rng, 0.75f))
                        b.type = Block::snow;
                    else
                        b.type = Block::ice;

                    // Rubies can generate deep in the stone layer
                    if (y > worldGen.rubyThreshold)
                    {
                        if (getRandomChance(rng, worldGen.rubyChance))
                            b.type = Block::snowSapphire;
                    }
                    // Not deep enough for rubies, but copper and other ores could still generate
                    else if (y > worldGen.oreThreshold)
                    {
                        // Add tundra ore in the future
                    }
                }

                // If we are higher up in the desert, sand generates instead of dirt and grass
                else if (y >= dirtLayer[x] && biomeNoise[x] > worldGen.minTundraThreshold && biomeNoise[x] < worldGen.maxTundraThreshold)
                {
                    b.type = Block::snow;

                    // Grass dirt, and clay blocks can generate near biome edges
                    if (getRandomChance(rng, blendChance))
                    {
                        if (y == dirtLayer[x])
                            b.type = Block::grassBlock;
                        else
                        {
                            b.type = Block::dirt;

                            if (y > worldGen.clayThreshold)
                            {
                                if (getRandomChance(rng, worldGen.clayChance))
                                    b.type = Block::clay;
                            }
                        }
                    }
                }

                b.randIndex = std::rand() % 4;

                gameMap.getBlockUnsafe(x, y) = b;

                gameMap.getWallBlockUnsafe(x, y) = b;
            }
        }
    };






    generateStoneLayer();
    generateDirtLayer();
    generateGrasslands();
    generateDesert();
    generateTundra();






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