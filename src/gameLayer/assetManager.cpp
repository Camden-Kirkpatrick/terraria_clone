#include "assetManager.hpp"
#include <raylib.h>

void AssetManager::loadAll()
{
	dirt = LoadTexture(RESOURCES_PATH "dirt.png");
	stone = LoadTexture(RESOURCES_PATH "stone.png");
}