#include "saveMap.hpp"
#include <asserts.hpp>
#include <iostream>

//const int VERSION = 2;
//
//struct BlockSaveRepresentation1
//{
//	std::uint16_t type = 0;
//	std::uint8_t randIndex = 0;
//
//	Block toBlock()
//	{
//		Block b;
//		b.type = type;
//		b.randIndex = randIndex;
//
//		return b;
//	}
//};
//
//struct BlockSaveRepresentation2
//{
//	std::uint16_t type = 0;
//	std::uint8_t randIndex = 0;
//	std::uint8_t durability = 1;
//
//	Block toBlock()
//	{
//		Block b;
//		b.type = type;
//		b.randIndex = randIndex;
//		b.durability = durability;
//
//		return b;
//	}
//};
//
//BlockSaveRepresentation2 toBlockRepresentation(Block b)
//{
//	BlockSaveRepresentation2 r;
//	r.type = b.type;
//	r.randIndex = b.randIndex;
//	r.durability = b.durability;
//
//	return r;
//}

// Write binary block data to a file
bool saveBlockDataToFile(const std::vector<Block> &blocks, std::vector<Block> wallBlocks, int w, int h, const char* fileName)
{
	std::ofstream f(fileName, std::ios::binary);

	if (!f.is_open())
		return false;

	permaAssertDevelopement(blocks.size() == w * h);
	permaAssertDevelopement(blocks.size() != 0);
	if (blocks.size() != w * h)
		return false;
	if (blocks.size() == 0)
		return false;

	permaAssertDevelopement(wallBlocks.size() == w * h);
	permaAssertDevelopement(wallBlocks.size() != 0);
	if (wallBlocks.size() != w * h)
		return false;
	if (wallBlocks.size() == 0)
		return false;

	//f.write((const char*)&VERSION, sizeof(VERSION));
	// Write the dimensions, so the loader knows how many blocks to read
	f.write((const char*)&w, sizeof(w));
	f.write((const char*)&h, sizeof(h));

	// Dump the entire block array as one contiguous stream of bytes
	f.write((const char*)blocks.data(), sizeof(Block) * blocks.size());
	f.write((const char*)wallBlocks.data(), sizeof(Block) * wallBlocks.size());

	//for (int i = 0; i < blocks.size(); i++)
	//{
	//	auto b = toBlockRepresentation(blocks[i]);
	//	auto wb = toBlockRepresentation(wallBlocks[i]);
	//	f.write((const char*)&b, sizeof(b));
	//	f.write((const char*)&wb, sizeof(wb));
	//}

	f.close();

	return true;
}

// Read binary block data from a file
bool loadBlockDataFromFile(std::vector<Block> &blocks, std::vector<Block> &wallBlocks, int &w, int &h, const char* fileName)
{
	// Reset outputs up front so a failed load leaves the caller with a clean state
	blocks.clear();
	w = 0;
	h = 0;

	std::ifstream f(fileName, std::ios::binary);

	if (!f.is_open())
		return false;

	int version = 0;

	//f.read((char*)&version, sizeof(version));
	// Get the width and height so we know how many blocks to allocate
	f.read((char*)&w, sizeof(w));
	f.read((char*)&h, sizeof(h));

	// Reject if the header read failed (file too short / not a save file) or if
	// the dimensions are zero/negative
	if (!f || w <= 0 || h <= 0)
	{
		f.close();
		return false;
	}

	// Guard against absurd dimensions from a corrupt or malicious file, which
	// would otherwise cause a huge allocation below
	if (w > 10000 || h > 10000)
	{
		f.close();
		return false;
	}

	//switch (version)
	//{
	//	case 1:
	//	{
	//		size_t blockCount = w * h;
	//		blocks.resize(blockCount);
	//		wallBlocks.resize(blockCount);

	//		for (int i = 0; i < blockCount; i++)
	//		{
	//			BlockSaveRepresentation1 read, readWall;
	//			f.read((char*)&read, sizeof(read));
	//			f.read((char*)&readWall, sizeof(readWall));

	//			if (!f)
	//			{
	//				blocks.clear();
	//				wallBlocks.clear();
	//				w = 0;
	//				h = 0;
	//				f.close();
	//				return false;
	//			}

	//			blocks[i] = read.toBlock();
	//			wallBlocks[i] = readWall.toBlock();
	//		}

	//		break;
	//	}

	//	case 2:
	//	{
	//		size_t blockCount = w * h;
	//		blocks.resize(blockCount);
	//		wallBlocks.resize(blockCount);

	//		for (int i = 0; i < blockCount; i++)
	//		{
	//			BlockSaveRepresentation2 read, readWall;
	//			f.read((char*)&read, sizeof(read));
	//			f.read((char*)&readWall, sizeof(readWall));

	//			if (!f)
	//			{
	//				blocks.clear();
	//				wallBlocks.clear();
	//				w = 0;
	//				h = 0;
	//				f.close();
	//				return false;
	//			}

	//			blocks[i] = read.toBlock();
	//			wallBlocks[i] = readWall.toBlock();
	//		}

	//		break;
	//	}

	//	// Wrong version
	//	default:
	//	{
	//		w = 0;
	//		h = 0;
	//		f.close();
	//		return false;
	//	}
	//}

	size_t blockCount = w * h;
	blocks.resize(blockCount);
	wallBlocks.resize(blockCount);

	// Read the entire stream of bytes back into our vectors
	f.read((char*)blocks.data(), sizeof(Block) * blockCount);
	f.read((char*)wallBlocks.data(), sizeof(Block) * blockCount);

	// Partial read = truncated/corrupt file; wipe the partially-filled vector
	if (!f)
	{
		blocks.clear();
		w = 0;
		h = 0;
		f.close();
		return false;
	}

	// Ensure all blocks have a valid type
	for (int i = 0; i < blocks.size(); i++)
		blocks[i].sanitize();

	f.close();
	return true;
}