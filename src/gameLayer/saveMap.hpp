#pragma once
#include <vector>
#include <fstream>
#include "blocks.hpp"

bool saveBlockDataToFile(std::vector<Block> blocks, std::vector<Block> wallBlocks, int w, int h, const char* fileName);

bool loadBlockDataFromFile(std::vector<Block> &blocks, std::vector<Block> &wallBlocks, int &w, int &h, const char* fileName);