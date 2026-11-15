#include "assetManager.hpp"
#include <raylib.h>
#include <string>

void AssetManager::loadAll()
{
	// Main texture atlas
	textures = LoadTexture(RESOURCES_PATH "texturesWithBackgroundVersion.png");
	// The block selection frame texture
	frame = LoadTexture(RESOURCES_PATH "frame.png");
	// The wood logs texture atlas (for tree blocks)
	woodLogs = LoadTexture(RESOURCES_PATH "treetextures.png");
	player = LoadTexture(RESOURCES_PATH "player.png");
	slime = LoadTexture(RESOURCES_PATH "slime.png");
}

void AssetManager::unloadAll()
{
	textures = {};
	frame = {};
}

void AssetManager::loadTexturePack(std::string texturePack)
{
	std::string filePath = "../texturePacks/" + texturePack + "/";

	textures = LoadTexture((RESOURCES_PATH + filePath + "texturesWithBackgroundVersion.png").c_str());
	frame = LoadTexture((RESOURCES_PATH + filePath + "frame.png").c_str());
}