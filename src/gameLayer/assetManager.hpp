#pragma once
#include <raylib.h>

struct AssetManager
{
	Texture2D textures = {};
	Texture2D frame = {};
	Texture2D woodLogs = {};

	void loadAll();
};