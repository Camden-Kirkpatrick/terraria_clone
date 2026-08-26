#include "assetManager.hpp"
#include <raylib.h>

void AssetManager::loadAll()
{
	// Main texture atlas
	textures = LoadTexture(RESOURCES_PATH "texturesWithBackgroundVersion.png");
	// The block selection frame texture
	frame = LoadTexture(RESOURCES_PATH "frame.png");
	// The wood logs texture atlas (for tree blocks)
	woodLogs = LoadTexture(RESOURCES_PATH "treetextures.png");
	player = LoadTexture(RESOURCES_PATH "player.png");
}