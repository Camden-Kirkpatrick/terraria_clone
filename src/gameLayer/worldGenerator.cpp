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

std::vector<float> savedTerrainNoise;

void resetWorldGen()
{
    worldGen.avgWorldHeight = 0;
    worldGen.wallStartDepth = 10;

    // Terrain settings
    worldGen.terrainOctaves = 1;
    worldGen.terrainFrequency = 0.002f;
    worldGen.terrainBlendZone = 0.045f;

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
    worldGen.biomeOctaves = 1;
    worldGen.biomeFrequency = 0.00033;
    // How many tiles out from a biome border the blending reaches.
    // With 40, only columns within 40 tiles of an actual biome change blend toward
    // the neighboring biome's blocks; columns farther out stay pure (blendChance hits 0).
    worldGen.biomeBlendRadius = 40;
    worldGen.blendBiomes = true;
    // Desert settings
    worldGen.minDesertThreshold = 0.0f;
    worldGen.maxDesertThreshold = 0.4f;
    // Tundra settings
    worldGen.minTundraThreshold = 0.6f;
    worldGen.maxTundraThreshold = 0.8f;

    // Cave settings
    worldGen.generateCaves = true;
    worldGen.caveOctaves = 4; // 8
    worldGen.caveFrequency = 0.01f; // 0.004f
    // When the cave noise is in this range, caves will generate
    worldGen.minCaveThreshold = 0.60f;
    worldGen.maxCaveThreshold = 0.80f;
    // Above this noise value caves open to the surface; below it they get a solid
    // cap up to maxCaveCeilingDepth tiles thick (ramps smoothly, no hard seam).
    worldGen.caveOpenThreshold = 0.70f;
    worldGen.maxCaveCeilingDepth = 100.0f;

    // Special block settings
    // Ores
    worldGen.generateOre = true;
    worldGen.oreThreshold = 375;
    worldGen.goldChance = 0.033f;
    worldGen.ironChance = 0.95f;
    worldGen.copperChance = 0.925f;
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
    worldGen.treeSpawnChance = 0.05f;
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
// 0.0 = pure plains, 1.0 = pure mountains, values between = blend.
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

Biome biomeFromNoise(float biomeNoise)
{
    if (biomeNoise > worldGen.minDesertThreshold && biomeNoise < worldGen.maxDesertThreshold)
        return Biome::Desert;
    else if (biomeNoise > worldGen.minTundraThreshold && biomeNoise < worldGen.maxTundraThreshold)
        return Biome::Tundra;
    else
        return Biome::Grasslands;
}

void generateWorld(GameMap& gameMap, const int WIDTH, const int HEIGHT, int seed, bool resetWrldGen)
{
    if (resetWrldGen)
        resetWorldGen();

    gameMap.create(WIDTH, HEIGHT);

    std::ranlux24_base rng;

    std::vector<Biome> biomeId(WIDTH);
    std::vector<int> distToBorder(WIDTH);
    std::vector<Biome> neighborBiome(WIDTH);

    // Leaf textures for trees
    Structure leaves1;
    loadBlockDataFromFile(
        leaves1.structureBlocks,
        leaves1.structureWallBlocks,
        leaves1.w,
        leaves1.h,
        RESOURCES_PATH "structures/leaves1.bin"
    );

    Structure leaves2;
    loadBlockDataFromFile(
        leaves2.structureBlocks,
        leaves2.structureWallBlocks,
        leaves2.w,
        leaves2.h,
        RESOURCES_PATH "structures/leaves2.bin"
    );

    Structure leaves3;
    loadBlockDataFromFile(
        leaves3.structureBlocks,
        leaves3.structureWallBlocks,
        leaves3.w,
        leaves3.h,
        RESOURCES_PATH "structures/leaves3.bin"
    );

    Structure leaves4;
    loadBlockDataFromFile(
        leaves4.structureBlocks,
        leaves4.structureWallBlocks,
        leaves4.w,
        leaves4.h,
        RESOURCES_PATH "structures/leaves4.bin"
    );

    Structure leaves5;
    loadBlockDataFromFile(
        leaves5.structureBlocks,
        leaves5.structureWallBlocks,
        leaves5.w,
        leaves5.h,
        RESOURCES_PATH "structures/leaves5.bin"
    );

    Structure leaves[5] = { leaves1, leaves2, leaves3, leaves4, leaves5 };

#pragma region generate_noise
    // Noise generators for different layers, biomes, and caves
    std::unique_ptr<FastNoiseSIMD> dirtNoiseGenerator(FastNoiseSIMD::NewFastNoiseSIMD());
    std::unique_ptr<FastNoiseSIMD> stoneNoiseGenerator(FastNoiseSIMD::NewFastNoiseSIMD());
    std::unique_ptr<FastNoiseSIMD> terrainNoiseGenerator(FastNoiseSIMD::NewFastNoiseSIMD());
    std::unique_ptr<FastNoiseSIMD> biomeNoiseGenerator(FastNoiseSIMD::NewFastNoiseSIMD());
    std::unique_ptr<FastNoiseSIMD> caveNoiseGenerator(FastNoiseSIMD::NewFastNoiseSIMD());
    std::unique_ptr<FastNoiseSIMD> oreNoiseGenerator(FastNoiseSIMD::NewFastNoiseSIMD());

    // Each generator gets a unique seed so their shapes don't match
    dirtNoiseGenerator->SetSeed(seed++);
    stoneNoiseGenerator->SetSeed(seed++);
    terrainNoiseGenerator->SetSeed(seed++);
    biomeNoiseGenerator->SetSeed(seed++);
    caveNoiseGenerator->SetSeed(seed++);
    oreNoiseGenerator->SetSeed(seed++);


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
    caveNoiseGenerator->SetFractalOctaves(1);
    caveNoiseGenerator->SetFrequency(0.04f);

    float* caveNoise2 = FastNoiseSIMD::GetEmptySet(WIDTH * HEIGHT);

    caveNoiseGenerator->FillNoiseSet(caveNoise2, 0, 0, 0, HEIGHT, WIDTH, 1);

    // Cave selector noise
    // Slow-frequency selector that picks which cave shape dominates in each region.
    // Lower frequency than the cave noises so a region commits to one style across
    // many tiles instead of flickering. Value near 0 = mostly caveNoise1's shape,
    // value near 1 = mostly caveNoise2's shape, in-between = smooth blend.
    caveNoiseGenerator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    caveNoiseGenerator->SetFractalOctaves(1);
    caveNoiseGenerator->SetFrequency(0.005f);

    float* caveSelectorNoise = FastNoiseSIMD::GetEmptySet(WIDTH * HEIGHT);

    caveNoiseGenerator->FillNoiseSet(caveSelectorNoise, 0, 0, 0, HEIGHT, WIDTH, 1);

    // Cave depth
    caveNoiseGenerator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    caveNoiseGenerator->SetFractalOctaves(1);
    caveNoiseGenerator->SetFrequency(0.0025f);

    float* caveDepthNoise = FastNoiseSIMD::GetEmptySet(WIDTH);

    caveNoiseGenerator->FillNoiseSet(caveDepthNoise, 0, 0, 0, WIDTH, 1, 1);

    // Noise for ores
    oreNoiseGenerator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
    oreNoiseGenerator->SetFractalOctaves(1);
    oreNoiseGenerator->SetFrequency(0.1f);

    float* oreNoise = FastNoiseSIMD::GetEmptySet(WIDTH * HEIGHT);

    oreNoiseGenerator->FillNoiseSet(oreNoise, 0, 0, 0, HEIGHT, WIDTH, 1);


    // Noise output is in range [-1, 1], remap to [0, 1]
    for (int i = 0; i < WIDTH; i++)
    {
        dirtPlainNoise[i] = (dirtPlainNoise[i] + 1) / 2;
        stonePlainNoise[i] = (stonePlainNoise[i] + 1) / 2;

        dirtMountainNoise[i] = (dirtMountainNoise[i] + 1) / 2;
        stoneMountainNoise[i] = (stoneMountainNoise[i] + 1) / 2;

        terrainNoise[i] = (terrainNoise[i] + 1) / 2;

        biomeNoise[i] = (biomeNoise[i] + 1) / 2;
        // Find out what every columns biome is 
        biomeId[i] = biomeFromNoise(biomeNoise[i]);

        caveDepthNoise[i] = (caveDepthNoise[i] + 1) / 2;
    }
    for (int i = 0; i < WIDTH * HEIGHT; i++)
    {
        caveNoise1[i] = (caveNoise1[i] + 1) / 2;
        caveNoise2[i] = (caveNoise2[i] + 1) / 2;
        caveSelectorNoise[i] = (caveSelectorNoise[i] + 1) / 2;

        oreNoise[i] = (oreNoise[i] + 1) / 2;
    }

    // For every column, find how many tiles away the nearest DIFFERENT biome is
    // (distToBorder) and which biome that is (neighborBiome). These drive the blend:
    // columns close to a border fade toward their neighbor's blocks, columns far from
    // any border stay pure. Distance is measured in real tiles, not noise values, so
    // blending only happens where biomes physically meet.
    for (int i = 0; i < WIDTH; i++)
    {
        Biome curBiome = biomeId[i];
        Biome leftBiome;
        Biome rightBiome;

        // Defaults assume "no border within range": distance maxed out at the blend
        // radius (which makes blendChance come out to 0), and neighbor is ourself.
        // The search below only ever overrides these when it actually finds a border.
        distToBorder[i] = worldGen.biomeBlendRadius;
        neighborBiome[i] = curBiome;

        // Walk outward from this column one step at a time, checking the same distance
        // on BOTH sides each step (j tiles left and j tiles right). j starts at 1 because
        // the first step looks at the immediate neighbors one tile to the left and right
        // (distance 0 would just be this column again). Because j grows outward from 1,
        // the first differing biome we hit is guaranteed to be the nearest one, so we
        // record it and stop. We only care about borders within biomeBlendRadius, so the
        // loop never needs to look farther than that.
        for (int j = 1; j < worldGen.biomeBlendRadius; j++)
        {
            // Look j tiles to the left (skip if that would fall off the map edge).
            if (i - j >= 0)
            {
                leftBiome = biomeId[i - j];
                if (leftBiome != curBiome)
                {
                    distToBorder[i] = j;
                    neighborBiome[i] = leftBiome;
                    break;
                }
            }
            // Look j tiles to the right (skip if that would fall off the map edge).
            if (i + j < WIDTH)
            {
                rightBiome = biomeId[i + j];
                if (rightBiome != curBiome)
                {
                    distToBorder[i] = j;
                    neighborBiome[i] = rightBiome;
                    break;
                }
            }
        }
    }

    // Used for displaying the current type of terrain (plains/mountains)
    savedTerrainNoise.assign(terrainNoise, terrainNoise + WIDTH);

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
     //Blend the two cave shapes per-tile using the selector as the lerp weight.
     //Then a single band threshold on the result carves the actual caves.
    auto getFinalCaveNoise = [&](int x, int y)
    {
        return lerp(getCaveNoise1(x, y), getCaveNoise2(x, y), getCaveSelectorNoise(x, y));
    };

    auto getOreNoise = [&](int x, int y)
    {
        return oreNoise[WIDTH * y + x];
    };
#pragma endregion

    std::vector<int> stoneLayer(WIDTH, 0);

    // Computes the stone-surface height for every column and stores it in stoneLayer[x].
    // This is just the heightmap pass: no blocks are placed here. Later passes read
    // stoneLayer[x] to know where the stone begins in each column.
    auto generateStoneLayer = [&]()
    {
        for (int x = 0; x < WIDTH; x++)
        {
            // Two candidate stone heights for this column: one if it were pure plains,
            // one if it were pure mountains. Each is a lerp across that terrain type's
            // [min, max] range, driven by its own per-column noise. We compute both up
            // front because the blend-zone branch below needs to interpolate between them.
            int stonePlainStart = lerp(worldGen.minStonePlainStart, worldGen.maxStonePlainStart, stonePlainNoise[x]);
            int stoneMountainStart = lerp(worldGen.minStoneMountainStart, worldGen.maxStoneMountainStart, stoneMountainNoise[x]);

            int stoneStart;

            // t: 0 = pure plains, 1 = pure mountains, in-between = blend zone.
            float t = terrainBlend(terrainNoise[x]);

            // Plains
            if (t <= 0.0f)
            {
                // Extra height variation only kicks in for the rare LOW tail of terrainNoise.
                // Simplex noise clusters around 0.5, so values below variationStart are
                // uncommon - only those columns get dramatic hills, the rest stay flat.
                const float variationStart = 0.33f;   // below this, variation ramps in
                const float maxStoneOffset = 12.0f;   // most we shift the stone range by

                // depth = how deep into the rare tail this column is, as a 0..1 fraction.
                // terrainNoise == variationStart -> 0 (no variation); == 0 -> 1 (max variation).
                // Dividing by variationStart rescales [0, variationStart] to [0, 1].
                float depth = 0.0f;
                if (terrainNoise[x] < variationStart)
                    depth = (variationStart - terrainNoise[x]) / variationStart;

                float stoneOffset = depth * maxStoneOffset;

                // Subtract the offset from the MIN so stoneStart can drop below its normal
                // floor. Smaller stoneStart = higher surface = taller terrain, so rare
                // columns spike up into hills while neighbours stay low.
                stoneStart = lerp(worldGen.minStonePlainStart - stoneOffset, worldGen.maxStonePlainStart, stonePlainNoise[x]);
            }
            // Mountains
            else if (t >= 1.0f)
            {
                // Mirror of the plains branch, but variation comes from the rare HIGH tail
                // of terrainNoise, and the offsets are bigger for more dramatic peaks.
                const float variationStart = 0.66f;   // above this, variation ramps in
                const float maxStoneOffset = 30.0f;

                // depth measured into the HIGH tail: variationStart -> 0, 1.0 -> 1.
                // Dividing by (1 - variationStart) rescales [variationStart, 1] to [0, 1].
                float depth = 0.0f;
                if (terrainNoise[x] > variationStart)
                    depth = (terrainNoise[x] - variationStart) / (1.0f - variationStart);

                float stoneOffset = depth * maxStoneOffset;

                // Same direction as plains (subtract from min -> surface goes up), just a
                // larger offset so peaks reach higher.
                stoneStart = lerp(worldGen.minStoneMountainStart - stoneOffset, worldGen.maxStoneMountainStart, stoneMountainNoise[x]);
            }
            // Blend zone: smoothly mix the plains and mountains heights by t so the two
            // terrain types meet with no hard seam.
            else
                stoneStart = lerp(stonePlainStart, stoneMountainStart, t);

            // Store the result for the dirt pass and the biome passes to read.
            stoneLayer[x] = stoneStart;
        }
    };

    std::vector<int> dirtLayer(WIDTH, 0);

    // Computes the dirt-surface height for every column and stores it in dirtLayer[x].
    // dirtLayer[x] is the y of the topmost dirt/surface block; it sits dirtThickness
    // tiles above stoneLayer[x]. Like the stone pass, this only computes heights.
    auto generateDirtLayer = [&]()
    {
        for (int x = 0; x < WIDTH; x++)
        {
            // Candidate dirt thicknesses for pure plains vs pure mountains, same idea
            // as the stone pass: compute both, then pick or blend below.
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

                // Widen only the MAX (never the min) so the dirt layer can get thicker on
                // rare columns but can never go negative - a negative thickness would push
                // dirtStart below stoneStart and wipe out the surface.
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

                // Same as plains: widen only the max so mountain dirt can get thick without
                // ever inverting.
                dirtThickness = lerp(worldGen.minDirtMountainThickness, worldGen.maxDirtMountainThickness + dirtOffset, dirtMountainNoise[x]);
            }
            // Blend zone between plains and mountains.
            else
                dirtThickness = lerp(dirtPlainThickness, dirtMountainThickness, t);

            // Dirt sits on top of stone. Smaller y = higher up, so subtracting the
            // thickness from the stone surface gives the y of the highest dirt block.
            dirtStart = stoneLayer[x] - dirtThickness;

            dirtLayer[x] = dirtStart;
        }
    };



    // Return the correct block for the world, given a biome, and a position
    auto blockFor = [&](Biome biome, int x, int y)
    {
        Block b;

        if (biome == Biome::Grasslands)
        {
            if (y > stoneLayer[x])
            {
                b.type = Block::stone;
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

            // Chance for grass to generate on grass blocks
            else if (y == dirtLayer[x] - 1)
            {
                if (getRandomChance(rng, 0.5f))
                    b.type = Block::grass;
            }
        }

        else if (biome == Biome::Desert)
        {
            // If we are in the stone layer, use the correct desert blocks
            if (y > stoneLayer[x])
            {
                if (getRandomChance(rng, 0.5f))
                    b.type = Block::sand;
                else
                    b.type = Block::sandStone;

                // Rubies can generate deep in the stone layer
                if (y > worldGen.rubyThreshold)
                {
                    if (getRandomChance(rng, worldGen.rubyChance))
                        b.type = Block::sandRuby;
                }
            }

            // If we are higher up in the desert, sand generates instead of dirt and grass
            else if (y >= dirtLayer[x])
                b.type = Block::sand;
        }

        else if (biome == Biome::Tundra)
        {
            if (y > stoneLayer[x])
            {
                if (getRandomChance(rng, 0.75f))
                    b.type = Block::snow;
                else
                    b.type = Block::ice;

                // Sapphires can generate deep in the stone layer
                if (y > worldGen.rubyThreshold)
                {
                    if (getRandomChance(rng, worldGen.rubyChance))
                        b.type = Block::snowSapphire;
                }
            }

            // If we are higher up in the tundra, snow generates instead of dirt and grass
            else if (y >= dirtLayer[x])
                b.type = Block::snow;
        }

        return b;
    };
    

    auto generateBiome = [&](Biome biome)
    {
        float blendChance = 0.0f;

        for (int x = 0; x < WIDTH; x++)
        {
            // Reseed per column so each column's blocks depend only on (seed, x): a terrain
            // edit in one column can't shift another, and a column can be regenerated on its own.
            // The +BIOME_OFFSET keeps this pass's rolls from matching the cave/tree passes, which
            // seed from the same column index.
            rng.seed(seed + x + BIOME_OFFSET);

            // Not the desired biome, so skip this column
            if (biomeId[x] != biome)
                continue;

            if (worldGen.blendBiomes)
            {
                // Probability of placing the neighboring biome's blocks instead of this biome's.
                // Based on distance (in tiles) to the nearest real biome border, and capped at 0.5
                // so the seam is a 50/50 mix: both biomes blend toward each other and meet halfway
                // instead of overshooting and swapping (which would make the transition blend twice).
                // e.g. distToBorder=1 (next to border) -> chance~0.5, distToBorder=blendRadius/2 -> chance=0.25, distToBorder=blendRadius (no border in range) -> chance=0.0
                blendChance = 0.5f * (1.0f - (float)distToBorder[x] / worldGen.biomeBlendRadius);
            }

            for (int y = 0; y < HEIGHT; y++)
            {
                Block b;

                b = blockFor(biomeId[x], x, y);
                b.randIndex = getRandomInt(rng, 0, 3);

                if (worldGen.blendBiomes)
                {
                    if (getRandomChance(rng, blendChance))
                        b = blockFor(neighborBiome[x], x, y);
                }

                gameMap.getBlockUnsafe(x, y) = b;
                if (y >= dirtLayer[x] + worldGen.wallStartDepth)
                    gameMap.getWallBlockUnsafe(x, y) = b;
            }
        }
    };



    auto generateOres = [&]()
    {
        if (worldGen.generateOre)
        {
            float blendChance = 0.0f;

            for (int x = 0; x < WIDTH; x++)
            {
                rng.seed(seed + x + ORE_OFFSET);

                if (worldGen.blendBiomes)
                    blendChance = 0.5f * (1.0f - (float)distToBorder[x] / worldGen.biomeBlendRadius);

                for (int y = worldGen.oreThreshold; y < HEIGHT; y++)
                {
                    Block b;
                    bool generateIronOre = false;
                    bool generateGoldOre = false;
                    bool generateCopperOre = false;

                    // Since gold and iron generate when the noise is low (gold), or high (iron), they generate in their own veins
                    if (biomeId[x] == Biome::Grasslands)
                    {
                        // Gold is slightly more rare than iron
                        generateGoldOre = (
                            getOreNoise(x, y) <  worldGen.goldChance
                        );

                        generateIronOre = (
                            getOreNoise(x, y) > worldGen.ironChance
                        );

                        if (generateGoldOre)
                            b.type = Block::gold;
                        else if (generateIronOre)
                            b.type = Block::iron;
                    }

                    else if (biomeId[x] == Biome::Desert)
                    {
                        generateCopperOre = (
                            getOreNoise(x, y) > worldGen.copperChance
                        );

                        if (generateCopperOre)
                            b.type = Block::copper;
                    }

                    // This block is not an ore, so ignore it
                    if (!generateIronOre && !generateGoldOre && !generateCopperOre)
                        continue;

                    b.randIndex = getRandomInt(rng, 0, 3);

                    gameMap.getBlockUnsafe(x, y) = b;
                }
            }
        }
    };



    auto generateCaves = [&]()
    {
        float blendChance = 0.0f;
        int yStart = 0;

        for (int x = 0; x < WIDTH; x++)
        {
            rng.seed(seed + x + CAVE_OFFSET);

            if (worldGen.blendBiomes)
                blendChance = 0.5f * (1.0f - (float)distToBorder[x] / worldGen.biomeBlendRadius);

            // Decide how far below the surface this column starts carving caves, so
            // caves only sometimes break through to the surface instead of everywhere.
            // caveDepthNoise is a low-frequency, per-column value in [0, 1]:
            //   - high noise (>= caveOpenThreshold) -> column is "open", caves can reach the surface
            //   - low  noise                        -> column is "capped" by solid ground
            //
            // t is the cap amount as a 0..1 weight. Subtracting the noise from the
            // threshold measures how far *below* the threshold we are, and dividing by
            // the threshold normalizes that to 0..1 so t hits exactly 0 right at the
            // threshold and 1 when the noise bottoms out at 0. std::max clamps anything
            // above the threshold (a negative result) to 0 -> fully open. Because t
            // reaches 0 *at* the threshold, the cap ramps smoothly into the open regions
            // with no hard vertical seam. (caveDepthNoise being low frequency keeps the
            // ramp gradual from one column to the next.)
            float t = std::max((worldGen.caveOpenThreshold - caveDepthNoise[x]) / worldGen.caveOpenThreshold, 0.0f);

            // Scale the 0..1 cap weight into a tile offset. maxCaveCeilingDepth is the
            // thickest possible roof (reached only where noise is 0); round to a whole
            // number of tiles.
            int offset = std::round(t * worldGen.maxCaveCeilingDepth);

            // Offset is measured down from this column's surface, so the solid cap
            // follows the terrain instead of sitting at a fixed world height.
            yStart = dirtLayer[x] + offset;

            // Start at yStart (surface + cap), so we ignore all air blocks above it
            for (int y = yStart; y < HEIGHT; y++)
            {
                // Band threshold: cave appears only when the *blended* noise lands in the
                // cave band. AND-ing two separate band checks would give intersection
                // (both noises agree); lerp-then-threshold gives a smooth morph between
                // two cave styles across regions.
                if (worldGen.generateCaves)
                {
                    bool generateCave = (
                        getFinalCaveNoise(x, y) < worldGen.maxCaveThreshold && getFinalCaveNoise(x, y) > worldGen.minCaveThreshold
                        //getCaveNoise2(x, y) < worldGen.maxCaveThreshold && getCaveNoise2(x, y) > worldGen.minCaveThreshold
                    );

                    if (!generateCave)
                        continue;

                    // Prevent caves from opening up to the void / edge of the map
                    if (y == HEIGHT - 1 || x == 0 || x == WIDTH - 1)
                        continue;

                    // Cave generation
                    Block b;
                    b.type = Block::air;
                    gameMap.getBlockUnsafe(x, y) = b;

                    if (y >= dirtLayer[x] + worldGen.wallStartDepth)
                    {
                        // The background block shouldn't be air in caves, but the foreground block should be air
                        Block background;

                        background = blockFor(biomeId[x], x, y);

                        if (worldGen.blendBiomes)
                        {
                            if (getRandomChance(rng, blendChance))
                                background = blockFor(neighborBiome[x], x, y);
                        }

                        background.randIndex = getRandomInt(rng, 0, 3);
                        gameMap.getWallBlockUnsafe(x, y) = background;
                    }
                }
            }
        }
    };



    auto generateTunnels = [&]()
    {
        rng.seed(seed + TUNNEL_OFFSET);

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
    };



    auto generateTrees = [&]()
    {
        if (worldGen.generateTrees)
        {
            for (int x = 0; x < WIDTH; x++)
            {
                rng.seed(seed + x + TREE_OFFSET);

                if (getRandomChance(rng, worldGen.treeSpawnChance))
                {
                    // Pick a random set of leaves
                    int leavesIndex = getRandomInt(rng, 0, 4);
                    Structure treeLeaves = leaves[leavesIndex];
                    
                    // Start at the surface (dirtLayer[x]), so we ignore all air blocks
                    for (int y = dirtLayer[x]; y < HEIGHT; y++)
                    {
                        // Get the current block type
                        uint16_t type = gameMap.getBlockType(x, y);
                        // Trees can only generate if there is a grass block under them
                        if (type != Block::grassBlock)
                            break;
                        else
                        {
                            Vector2 spawnPos = { (float)x, (float)y };
                            int minTreeHeight;
                            int maxTreeHeight;

                            // Depending on the leaves being used, the tree height changes
                            // More leaves = taller trees, Less leaves = smaller trees
                            switch (leavesIndex)
                            {
                                case 0:
                                    minTreeHeight = 3;
                                    maxTreeHeight = 5;
                                    break;

                                case 1:
                                    minTreeHeight = 5;
                                    maxTreeHeight = 7;
                                    break;

                                case 2:
                                    minTreeHeight = 7;
                                    maxTreeHeight = 9;
                                    break;

                                case 3:
                                    minTreeHeight = 9;
                                    maxTreeHeight = 11;
                                    break;

                                case 4:
                                    minTreeHeight = 11;
                                    maxTreeHeight = 13;
                                    break;
                                
                                // Invalid leavesIndex
                                default:
                                    minTreeHeight = 0;
                                    maxTreeHeight = 0;
                                    break;
                            }

                            // How many wood logs tall should the tree be?
                            int treeHeight = getRandomInt(rng, minTreeHeight, maxTreeHeight);

                            // Place the wood logs in the world
                            for (int i = y - 1; i >= y - treeHeight; i--)
                                gameMap.getBlockUnsafe(x, i).type = Block::woodLog;

                            // The place where we paste the leaves (top-left corner of the structure)
                            spawnPos.x -= treeLeaves.w / 2;
                            spawnPos.y = y - treeHeight - treeLeaves.h;

                            treeLeaves.pasteIntoMap(gameMap, spawnPos);

                            // Leave a gap between trees
                            x += 9;
                            
                            // Break, since we only care about the surface block
                            break;
                        }
                    }
                }
            }
        }
    };



    // Generate the world
    generateStoneLayer();
    generateDirtLayer();

    int totalHeight = 0;

    for (int x = 0; x < WIDTH; x++)
        totalHeight += dirtLayer[x];

    worldGen.avgWorldHeight = std::round((float)totalHeight / WIDTH);

    generateBiome(Biome::Grasslands);
    generateBiome(Biome::Desert);
    generateBiome(Biome::Tundra);
    generateOres();
    generateCaves();
    generateTunnels();
    generateTrees();

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