#include "gameMain.hpp"
#include "assetManager.hpp"
#include "asserts.hpp"
#include "gameMap.hpp"
#include <raylib.h>
#include <fstream>
#include <iostream>

struct GameData
{
	GameMap gameMap;
	Camera2D camera;
	float cameraSpeed = 10.0f;
} gameData;

AssetManager assetManager;

bool initGame()
{
	assetManager.loadAll();

	// Create a 20x20 map
	gameData.gameMap.create(20, 20);

	// Add blocks to the map
	gameData.gameMap.getBlockUnsafe(0, 0).type = Block::dirt;
	gameData.gameMap.getBlockUnsafe(1, 1).type = Block::stone;
	gameData.gameMap.getBlockUnsafe(2, 2).type = Block::glass;
	gameData.gameMap.getBlockUnsafe(3, 3).type = Block::leaves;
	gameData.gameMap.getBlockUnsafe(4, 4).type = Block::platform;

	// Camera setup
	gameData.camera.target = { 0, 0 }; // the world-space point the camera looks at; starts at the map origin
	gameData.camera.rotation = 0.0f;   // no rotation
	gameData.camera.zoom = 100.0f;     // 1 world unit = 100 screen pixels
	
	return true;
}

bool updateGame()
{
	float deltaTime = GetFrameTime();
	if (deltaTime > 0.05f) deltaTime = 0.05f; // clamp to 20fps minimum

	// offset is the screen-space pixel that the target maps to.
	// Keeping it at the screen center means the camera's target always appears in the middle of the window.
	// This is recalculated every frame so it adjusts automatically if the window is resized.
	gameData.camera.offset = { GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };

	// Camera movement: shift the target (the world point we're looking at) at 300 units/sec.
	// Multiplying by deltaTime makes the speed framerate-independent.
	if (IsKeyDown(KEY_A)) { gameData.camera.target.x -= gameData.cameraSpeed * deltaTime; } // pan left
	if (IsKeyDown(KEY_D)) { gameData.camera.target.x += gameData.cameraSpeed * deltaTime; } // pan right
	if (IsKeyDown(KEY_W)) { gameData.camera.target.y -= gameData.cameraSpeed * deltaTime; } // pan up
	if (IsKeyDown(KEY_S)) { gameData.camera.target.y += gameData.cameraSpeed * deltaTime; } // pan down



	// Change the background color
	ClearBackground({ 75, 75, 150, 255 });

	// Everything drawn between BeginMode2D and EndMode2D is rendered in world space,
	// transformed through the camera (target, offset, zoom, rotation).
	BeginMode2D(gameData.camera);

	// Draw the map
	for (int y = 0; y < gameData.gameMap.h; y++)
	{
		for (int x = 0; x < gameData.gameMap.w; x++)
		{
			// Get the current block
			Block& b = gameData.gameMap.getBlockUnsafe(x, y);

			if (b.type != Block::air)
			{
				// Set block properties
				float size = 1; // 1 world unit per block; zoom scales this to 100x100 pixels on screen

				Rectangle textureUV;
				// Texture size
				textureUV.width = 32;
				textureUV.height = 32;
				// Sample the textures at these coordinates
				textureUV.x = b.type * 32; // this gives the horizontal offset into the texture atlas
				textureUV.y = 0; // The top row of the texture atlas is used

				// Draw the block
				DrawTexturePro(
					assetManager.textures,					// The whole texture atlas
					textureUV,								// This is the 32x32 region to read from in the texture atlas
					{ float(x), float(y), size, size },     // This is where we draw it on screen
					{ 0, 0 },
					0.0f,
					WHITE
				);
			}
		}
	}

	// Anything drawn after this (e.g. HUD) uses raw screen coordinates, unaffected by the camera.
	EndMode2D();

	return true;
}

void closeGame()
{
	std::ofstream f(RESOURCES_PATH "f.txt");
	f << "CLOSED\n";
	f.close();
}