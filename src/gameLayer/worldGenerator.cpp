#include "worldGenerator.hpp"
#include "randomStuff.hpp"

void generateWorld(GameMap& gameMap, int WIDTH, int HEIGHT, int SEED)
{
    gameMap.create(WIDTH, HEIGHT);

    // Use a random number generator populated with a seed
    std::ranlux24_base rng(SEED);

    const int MIN_TIME = 5;
    const int MAX_TIME = 50;
    // Y coordinate bounds that clamp where the dirt/stone layer boundaries can settle (y=0 is the top of the world)
    const int MIN_DIRT_HEIGHT = 50;
    const int MAX_DIRT_HEIGHT = 90;
    const int MIN_STONE_HEIGHT = 60;
    const int MAX_STONE_HEIGHT = 120;
    const bool SHOW_THRESHOLDS = false;
    const int GOLD_THRESHOLD = 30;
    const float GOLD_CHANCE = 0.01f;
   
    // A timer set with a random value. When the timer reaches 0, a new direction is selected.
    int dirtDirectionTimer = getRandomInt(rng, MIN_TIME, MAX_TIME);

    enum
    {
        STEEP_DECLINE = -3,
        DECLINE = -2,
        MILD_DECLINE = -1,
        FLAT_SURFACE = 0,
        MILD_INCLINE = 1,
        INCLINE = 2,
        STEEP_INCLINE = 3
    };

    // Different directions that dirt can be placed in
    int dirtDirection = getRandomInt(rng, STEEP_DECLINE, STEEP_INCLINE);

    int stoneDirectionTimer = getRandomInt(rng, MIN_TIME, MAX_TIME);
    int stoneDirection = getRandomInt(rng, STEEP_DECLINE, STEEP_INCLINE);

    // Thresholds for when the block type changes
    int dirtHeight = 70;
    int stoneHeight = 90;

    // Go through every block in the map
    for (int x = 0; x < WIDTH; x++)
    {
        // Subtract one from the timer for each horizontal distance made
        dirtDirectionTimer--;
        // Reset the timer and pick a new direction
        if (dirtDirectionTimer == 0)
        {
            dirtDirectionTimer = getRandomInt(rng, MIN_TIME, MAX_TIME);
            dirtDirection = getRandomInt(rng, STEEP_DECLINE, STEEP_INCLINE);
        }

        // Change dirtHeight based off of the dirtDirection
        switch (dirtDirection)
        {
        case STEEP_DECLINE:
            for (int i = 0; i < 3; i++)
                if (getRandomChance(rng, 0.25f))
                    dirtHeight++;
            break;

        case DECLINE:
            for (int i = 0; i < 2; i++)
                if (getRandomChance(rng, 0.25f))
                    dirtHeight++;
            break;

        case MILD_DECLINE:
            if (getRandomChance(rng, 0.25f))
                dirtHeight++;
            break;

        case MILD_INCLINE:
            if (getRandomChance(rng, 0.25f))
                dirtHeight--;
            break;

        case INCLINE:
            for (int i = 0; i < 2; i++)
                if (getRandomChance(rng, 0.25f))
                    dirtHeight--;
            break;

        case STEEP_INCLINE:
            for (int i = 0; i < 3; i++)
                if (getRandomChance(rng, 0.25f))
                    dirtHeight--;
            break;

        default:
            break;
        }

        // dirtHeight must be within this range
        if (dirtHeight < MIN_DIRT_HEIGHT) dirtHeight = MIN_DIRT_HEIGHT;
        if (dirtHeight > MAX_DIRT_HEIGHT) dirtHeight = MAX_DIRT_HEIGHT;
        

        // Same exact code, but for stone instead
        stoneDirectionTimer--;
        if (stoneDirectionTimer == 0)
        {
            stoneDirectionTimer = getRandomInt(rng, MIN_TIME, MAX_TIME);
            stoneDirection = getRandomInt(rng, STEEP_DECLINE, STEEP_INCLINE);
        }

        switch (stoneDirection)
        {
        case STEEP_DECLINE:
            for (int i = 0; i < 3; i++)
                if (getRandomChance(rng, 0.25f))
                    stoneHeight++;
            break;

        case DECLINE:
            for (int i = 0; i < 2; i++)
                if (getRandomChance(rng, 0.25f))
                    stoneHeight++;
            break;

        case MILD_DECLINE:
            if (getRandomChance(rng, 0.25f))
                stoneHeight++;
            break;

        case MILD_INCLINE:
            if (getRandomChance(rng, 0.25f))
                stoneHeight--;
            break;

        case INCLINE:
            for (int i = 0; i < 2; i++)
                if (getRandomChance(rng, 0.25f))
                    stoneHeight--;
            break;

        case STEEP_INCLINE:
            for (int i = 0; i < 3; i++)
                if (getRandomChance(rng, 0.25f))
                    stoneHeight--;
            break;
        default:
            break;
        }

        if (stoneHeight < MIN_STONE_HEIGHT) stoneHeight = MIN_STONE_HEIGHT;
        if (stoneHeight > MAX_STONE_HEIGHT) stoneHeight = MAX_STONE_HEIGHT;

        // Set the block type based on the HEIGHTs
        for (int y = 0; y < HEIGHT; y++)
        {
            Block b;

            // When y is above the stoneHight threshold, stone can generate
            if (y > stoneHeight)
            {
                // Gold can generate further down in the stone layer
                if (y > stoneHeight + GOLD_THRESHOLD)
                {
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

            if (SHOW_THRESHOLDS)
            {
                // Show the bounds for where dirt can generate
                // Highest point: y = MIN_DIRT_HEIGHT + 1 ->
                // Blocks at dirtHeight are always grass, since blocks are dirt only when when y > dirtHeight.
                // This means the highest dirt is found one below the highest grass block.
                // Lowest point: y = MAX_STONE_HEIGHT -> 
                // When stoneHeight == MAX_STONE_HEIGHT, blocks that are at MAX_STONE_HEIGHT are still dirt,
                // since stone only appears when y > stoneHeight.
                if (y == MIN_DIRT_HEIGHT + 1 || y == MAX_STONE_HEIGHT)
                    b.type = Block::goldBlock;

                // Show the highest point that stone can generate.
                // When stoneHeight == MIN_STONE_HEIGHT, blocks that are at MIN_STONE_HEIGHT are still dirt,
                // since stone only appears when y > stoneHeight.
                // This means that the highest stone is found one block below this.
                if (y == MIN_STONE_HEIGHT + 1)
                    b.type = Block::glass;
            }

            // Each block will use a random texture variation.
            b.randIndex = std::rand() % 4;

            gameMap.getBlockUnsafe(x, y) = b;
        }
    }
}