#include "saveMap.hpp"
#include <asserts.hpp>
#include <iostream>

// Format version for NEW saves. Bump by 1 whenever the on-disk layout below
// changes. Written as the first bytes of every file so the loader knows which
// reader (case) to use
const int VERSION = 1;

// FROZEN snapshot of the version-1 on-disk layout. NEVER edit this - real files
// were written in this exact byte layout. Only the fields that existed in v1
struct BlockSaveRepresentation1
{
	std::uint16_t type = 0;
	std::uint8_t randIndex = 0;

	// Disk record -> live Block. Fields missing from previous versions are left at
	// Block's defaults, so old saves auto-upgrade with sane values
	Block toBlock()
	{
		Block b;
		b.type = type;
		b.randIndex = randIndex;

		return b;
	}
};

// FROZEN snapshot of the version-2 layout = v1 + new variables
// struct BlockSaveRepresentation2 { ... };

// Pack a live Block into the CURRENT representation (matches VERSION)
// Used while saving. Change the return type when you bump to a new version
BlockSaveRepresentation1 toBlockRepresentation(Block b)
{
	BlockSaveRepresentation1 r;
	r.type = b.type;
	r.randIndex = b.randIndex;

	return r;
}

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

	// File layout: [VERSION][w][h], then per cell: block, then wall (interleaved)
	f.write((const char*)&VERSION, sizeof(VERSION));
	// Write the dimensions, so the loader knows how many blocks to read
	f.write((const char*)&w, sizeof(w));
	f.write((const char*)&h, sizeof(h));

	// Two records per cell - foreground block, then the wall behind it
	for (int i = 0; i < blocks.size(); i++)
	{
		auto b = toBlockRepresentation(blocks[i]);
		auto wb = toBlockRepresentation(wallBlocks[i]);
		f.write((const char*)&b, sizeof(b));
		f.write((const char*)&wb, sizeof(wb));
	}

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

	// Read the format stamp FIRST - it decides which reader runs below
	int version = 0;
	f.read((char*)&version, sizeof(version));

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

	// One case per format version. Each reads with its OWN frozen representation, so
	// old files are read exactly as written. Keep every old case forever.
	switch (version)
	{
		// VERSION = 1
		case 1:
		{
			size_t blockCount = w * h;
			blocks.resize(blockCount);
			wallBlocks.resize(blockCount);

			for (int i = 0; i < blockCount; i++)
			{
				BlockSaveRepresentation1 read, readWall;
				f.read((char*)&read, sizeof(read));
				f.read((char*)&readWall, sizeof(readWall));

				if (!f)
				{
					blocks.clear();
					wallBlocks.clear();
					w = 0;
					h = 0;
					f.close();
					return false;
				}

				blocks[i] = read.toBlock();
				wallBlocks[i] = readWall.toBlock();
			}

			break;
		}

		// Unknown version - we don't know this layout, fail safely
		default:
		{
			w = 0;
			h = 0;
			f.close();
			return false;
		}
	}

	// Clamp any out-of-range block type to air so a bad/stale type can't crash rendering
	for (int i = 0; i < blocks.size(); i++)
		blocks[i].sanitize();

	f.close();
	return true;
}