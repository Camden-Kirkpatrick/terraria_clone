#include "saveMap.hpp"
#include <asserts.hpp>

bool saveBlockDataToFile(std::vector<Block> blocks, int w, int h, const char* fileName)
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

	f.write((const char*)&w, sizeof(w));
	f.write((const char*)&h, sizeof(h));

	f.write((const char*)blocks.data(), sizeof(Block) * blocks.size());

	f.close();

	return true;
}