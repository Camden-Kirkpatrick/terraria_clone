#include "worldGenerator.hpp"
#include "randomStuff.hpp"

void generateWorld(GameMap& gameMap, int seed)
{
    const int w = 100;
    const int h = 100;

    gameMap.create(w, h);

    // stoneDepth: how many rows from the bottom are filled with stone (50 blocks)
    // dirtDepth:  how many rows sit on top of stone, filled with dirt (10 blocks)
    // Both are random-walked per column to create an uneven, natural surface.
    int stoneDepth = 50;
    int dirtDepth = 10;

    std::ranlux24_base rng(seed);

    for (int x = 0; x < w; x++)
    {
        // Randomly drift the layer thicknesses by -1, 0, or +1 each column.
        // This makes the terrain surface and stone depth vary organically across the map.
        stoneDepth = stoneDepth + getRandomInt(rng, -1, 1);
        dirtDepth = dirtDepth + getRandomInt(rng, -1, 1);

        for (int y = 0; y < h; y++)
        {
            Block b;

            // The surface line sits at y = h - (dirtDepth + stoneDepth).
            // Everything above it is air; everything at or below it is terrain.
            if      (y < h - (dirtDepth + stoneDepth)) {}                               // air
            else if (y == h - (dirtDepth + stoneDepth)) { b.type = Block::grassBlock; } // surface row
            else if (y < h - stoneDepth) { b.type = Block::dirt; }                      // dirt layer
            else
            {
                // Stone layer (bottom 50 rows).
                // 3% of stone cells become gold ore.
                if (getRandomChance(rng, 0.03f))
                    b.type = Block::gold;
                else
                    b.type = Block::stone;
            }

            gameMap.getBlockUnsafe(x, y) = b;
        }
    }
}