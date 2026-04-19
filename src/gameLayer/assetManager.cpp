#include "assetManager.hpp"
#include <raylib.h>

void AssetManager::loadAll()
{
	textures = LoadTexture(RESOURCES_PATH "textures.png");
	frame = LoadTexture(RESOURCES_PATH "frame.png");
	woodLogs = LoadTexture(RESOURCES_PATH "treetextures.png");
}