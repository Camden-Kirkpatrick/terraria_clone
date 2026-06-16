#include "saveMap.hpp"
#include <asserts.hpp>
#include <iostream>

bool saveBlockDataToFile(std::vector<Block> blocks, std::vector<Block> wallBlocks, int w, int h, const char* fileName)
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

	// Write the dimensions first so the loader knows how many blocks follow
	f.write((const char*)&w, sizeof(w));
	f.write((const char*)&h, sizeof(h));

	// Dump the entire block array as one contiguous byte blob
	f.write((const char*)blocks.data(), sizeof(Block) * blocks.size());
	f.write((const char*)wallBlocks.data(), sizeof(Block) * wallBlocks.size());

	f.close();

	return true;
}

bool loadBlockDataFromFile(std::vector<Block> &blocks, std::vector<Block> &wallBlocks, int &w, int &h, const char* fileName)
{
	// Reset outputs up front so a failed load leaves the caller with a clean state
	blocks.clear();
	w = 0;
	h = 0;

	std::ifstream f(fileName, std::ios::binary);

	if (!f.is_open())
		return false;

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

	size_t blockCount = w * h;
	blocks.resize(blockCount);

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