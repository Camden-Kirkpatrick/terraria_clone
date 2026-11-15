#pragma once
#include <raylib.h>
#include <string>

struct AssetManager
{
	Texture2D textures = {};
	Texture2D frame = {};
	Texture2D woodLogs = {};
	Texture2D player = {};
	Texture2D slime = {};

	void loadAll();
	void unloadAll();
	void loadTexturePack(std::string texturePack);
};