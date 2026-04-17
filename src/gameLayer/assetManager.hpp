#pragma once
#include <raylib.h>

struct AssetManager
{
	Texture2D dirt = {};
	Texture2D stone = {};
	Texture2D textures = {};

	void loadAll();
};