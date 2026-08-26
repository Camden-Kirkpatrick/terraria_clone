#pragma once
#include <raylib.h>

struct AssetManager
{
	Texture2D textures = {};
	Texture2D frame = {};
	Texture2D woodLogs = {};
	Texture2D player = {};

	void loadAll();
};