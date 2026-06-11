#pragma once
#include <vector>
#include <fstream>
#include "blocks.hpp"

bool saveBlockDataToFile(std::vector<Block> blocks, int w, int h, const char* fileName);

bool loadBlockDataFromFile(std::vector<Block> &blocks, int w, int h, const char* fileName);