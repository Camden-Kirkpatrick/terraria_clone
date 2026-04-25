#include "worldGenerator.hpp"
#include "randomStuff.hpp"

void generateWorld(GameMap& gameMap, int seed)
{
    const int w = 900;
    const int h = 500;

    gameMap.create(w, h);

    std::ranlux24_base rng(seed);

    int dirtDirectionTimer = getRandomInt(rng, 5, 50);
    int dirtDirection = getRandomInt(rng, -2, 2);

    int stoneDirectionTimer = getRandomInt(rng, 5, 50);
    int stoneDirection = getRandomInt(rng, -2, 2);

    int dirtHeight = 70;
    int stoneHeight = 90;

    for (int x = 0; x < w; x++)
    {
        dirtDirectionTimer--;
        if (dirtDirectionTimer <= 0)
        {
            dirtDirectionTimer = getRandomInt(rng, 5, 50);
            dirtDirection = getRandomInt(rng, -2, 2);
        }

        if (dirtDirection == -1)
        {
            if (getRandomChance(rng, 0.25f))
                dirtHeight--;
        }
        else if (dirtDirection == -2)
        {
            if (getRandomChance(rng, 0.25f))
                dirtHeight--;
            if (getRandomChance(rng, 0.25f))
                dirtHeight--;
        }
        else if (dirtDirection == 1)
        { 
            if (getRandomChance(rng, 0.25f))
                dirtHeight++;
        }
        else if (dirtDirection == 2)
        {
            if (getRandomChance(rng, 0.25f))
                dirtHeight++;
            if (getRandomChance(rng, 0.25f))
                dirtHeight++;
        }

        if (dirtHeight < 50) dirtHeight = 50;
        if (dirtHeight > 90) dirtHeight = 90;
        

        stoneDirectionTimer--;
        if (stoneDirectionTimer <= 0)
        {
            stoneDirectionTimer = getRandomInt(rng, 5, 50);
            stoneDirection = getRandomInt(rng, -2, 2);
        }

        if (stoneDirection == -1)
        {
            if (getRandomChance(rng, 0.25f))
                stoneHeight--;
        }
        else if (stoneDirection == -2)
        {
            if (getRandomChance(rng, 0.25f))
                stoneHeight--;
            if (getRandomChance(rng, 0.25f))
                stoneHeight--;
        }
        else if (stoneDirection == 1)
        {
            if (getRandomChance(rng, 0.25f))
                stoneHeight++;
        }
        else if (stoneDirection == 2)
        {
            if (getRandomChance(rng, 0.25f))
                stoneHeight++;
            if (getRandomChance(rng, 0.25f))
                stoneHeight++;
        }

        if (stoneHeight < 60) stoneHeight = 60;
        if (stoneHeight > 120) stoneHeight = 120;

        for (int y = 0; y < h; y++)
        {
            Block b;

            if (y > dirtHeight) b.type = Block::dirt;

            if (y == dirtHeight) b.type = Block::grassBlock;

            if (y > stoneHeight) b.type = Block::stone;

            gameMap.getBlockUnsafe(x, y) = b;
        }
    }

    
}