#pragma once
#include <cstdint>

struct Block
{
	enum
	{
		air = 0,
		dirt,
		grassBlock,
		stone,
		grass,
		sand,
		sandRuby,
		sandStone,
		woodPlank,
		stoneBricks,
		clay,
		woodLog,
		leaves,
		copper,
		iron,
		gold,
		copperBlock,
		ironBlock,
		goldBlock,
		bricks,
		snow,
		ice,
		rubyBlock,
		platform,
		workBench,
		glass,
		furnace,
		painting,
		sappling,
		snowBlueRuby,
		blueRubyBlock,
		door,
		jar,
		table,
		wordrobe,
		bookShelf,
		snowBricks,
		iceTable,
		iceWordrobe,
		iceBookShelf,
		icePlatform,
		sandTable,
		sandWordrobe,
		sandBookShelf,
		sandPlatform,
		woodenChest,
		iceChest,
		sandChest,
		boneChest,
		boneBricks,
		boneBench,
		boneWordrobe,
		boneBookShelf,
		bonePlatform,
		gravel,

		BLOCKS_COUNT,
	};

	// The type of block, which determines its texture and behavior. air = 0 means no block.
	std::uint16_t type = air;
	// This is used to select a random row in the texture atlas for blocks that have multiple textures (e.g. different types of dirt or stone).
	std::uint8_t randIndex = 0;

	// This array maps each block type to the column index in the texture atlas for its wall block texture.
	// A value of -1 means this block type doesn't have a wall block texture and should be skipped when drawing wall blocks.
	static constexpr int wallColumn[Block::BLOCKS_COUNT] = {
		-1, // air
		54, // dirt
		-1, // grassBlock
		55, // stone
		-1, // grass
		64, // sand
		57, // sandRuby (use the sand stone wall)
		57, // sandStone
		56, // woodPlank
		65, // stoneBricks
		54, // clay (use the dirt wall)
		-1, // woodLog
		-1, // leaves
		57, // copper (use the sand stone wall)
		55, // iron   (use the stone wall)
		55, // gold   (use the stone wall)
		60, // copperBlock
		61, // ironBlock
		62, // goldBlock
		58, // bricks
		63, // snow
		-1, // ice
		66, // rubyBlock
		-1, // platform
		-1, // workBench
		59, // glass
	};
};