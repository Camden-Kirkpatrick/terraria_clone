#include "gameMain.hpp"
#include "assetManager.hpp"
#include "asserts.hpp"
#include "gameMap.hpp"
#include <raylib.h>
#include <fstream>
#include <iostream>

struct GameData
{
	//float posX = 0;
	//float posY = 0;
	//int playerWidth = 50;
	//int playerHeight = 50;
	//Color c{255, 0, 200, 255};

	GameMap gameMap;
	Camera2D camera;
	float cameraSpeed = 10.0f;

}gameData;

AssetManager assetManager;

bool initGame()
{
	assetManager.loadAll();

	// Create a 20x20 map
	gameData.gameMap.create(10, 1);

	// Add blocks to the map
	//gameData.gameMap.getBlockUnsafe(0, 0).type = Block::dirt;
	//gameData.gameMap.getBlockUnsafe(1, 1).type = Block::dirt;
	//gameData.gameMap.getBlockUnsafe(2, 2).type = Block::dirt;
	//gameData.gameMap.getBlockUnsafe(3, 3).type = Block::dirt;
	//gameData.gameMap.getBlockUnsafe(4, 4).type = Block::dirt;

	// Draw a checkerboard pattern
	for (int y = 0; y < gameData.gameMap.h; y++)
	{
		for (int x = 0; x < gameData.gameMap.w; x++)
		{	
			if ((x + y) % 2 == 0)
				gameData.gameMap.getBlockUnsafe(x, y).type = Block::stone;
		}
	}

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

	//DrawText("TEST", 100, 100, 20, RED);

	//DrawRectangle(150, 150, 100, 100, { 255, 0, 0, 127 });
	//DrawRectangle(175, 175, 100, 100, { 0, 255, 0, 255 });



	//// Player movement
	//if (IsKeyDown(KEY_A)) { gameData.posX -= 200 * deltaTime; }
	//if (IsKeyDown(KEY_D)) { gameData.posX += 200 * deltaTime; }
	//if (IsKeyDown(KEY_W)) { gameData.posY -= 200 * deltaTime; }
	//if (IsKeyDown(KEY_S)) { gameData.posY += 200 * deltaTime; }



	//// Prevent the player from going out of bounds
	//if (gameData.posX < 0) gameData.posX = 0;
	//if (gameData.posX + gameData.playerWidth > WIN_WIDTH)
	//	gameData.posX = WIN_WIDTH - gameData.playerWidth;
	// 
	//if (gameData.posY < 0) gameData.posY = 0;
	//if (gameData.posY + gameData.playerHeight > WIN_HEIGHT)
	//	gameData.posY = WIN_HEIGHT - gameData.playerHeight;


	// Wrap the player to the left/right/top/bottom of the screen
    //if (gameData.posX + gameData.playerWidth > WIN_WIDTH)
	//	gameData.posX = 0;
	//if (gameData.posX < 0)
	//	gameData.posX = WIN_WIDTH - gameData.playerWidth;

	//if (gameData.posY + gameData.playerHeight > WIN_HEIGHT)
	//	gameData.posY = 0;
	//if (gameData.posY < 0)
	//	gameData.posY = WIN_HEIGHT - gameData.playerHeight;
	

	// Allow the player to wrap smoothy (pixel by pixel), instead of all at once
	//for (int i = gameData.posY; i < gameData.posY + gameData.playerHeight; i++)
	//{
	//	for (int j = gameData.posX; j < gameData.posX + gameData.playerWidth; j++)
	//	{
	//		int wrappedX = ((j % WIN_WIDTH) + WIN_WIDTH) % WIN_WIDTH;
	//		int wrappedY = ((i % WIN_HEIGHT) + WIN_HEIGHT) % WIN_HEIGHT;
	//		DrawPixel(wrappedX, wrappedY, gameData.c);
	//	}
	//}

	//DrawRectangle(gameData.posX, gameData.posY, gameData.playerWidth, gameData.playerHeight, gameData.c);



	//DrawTexturePro(
	//	assetManager.dirt,                                                         // The source texture to draw
	//	{ 0, 0, (float)assetManager.dirt.width, (float)assetManager.dirt.height }, // Source rectangle (which part of the texture to sample)
	//	{ 50, 50, 100, 100 },                                                      // Dest rectangle (where and how big to draw it on screen)
	//	{0, 0},                                                                    // Origin point for rotation (default {0,0} = top-left)
	//	0.0f,                                                                      // Rotation angle in degrees
	//	WHITE                                                                      // Tint color (WHITE = no tint, draw as-is)
	//);

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
				float size = 1;
				float posX = x * size;
				float posY = y * size;

				// Draw the block
				DrawTexturePro(
					assetManager.stone,
					Rectangle{ 0.0f, 0.0f, (float)assetManager.stone.width, (float)assetManager.stone.height },
					{ posX, posY, size, size },
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