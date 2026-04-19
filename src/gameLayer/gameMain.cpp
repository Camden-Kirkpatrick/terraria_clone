#include "gameMain.hpp"
#include "assetManager.hpp"
#include "asserts.hpp"
#include "gameMap.hpp"
#include "helpers.hpp"
#include "blocks.hpp"
#include <raylib.h>
#include <raymath.h>
#include <fstream>
#include <iostream>

struct GameData
{ 
	GameMap gameMap;
	Camera2D camera = {};
	float cameraSpeed = 15.0f;
	int hoverMode = 0; // 0 = block, 1 = wall block
} gameData;

AssetManager assetManager;

bool initGame()
{
	// Load assets (textures)
	assetManager.loadAll();

	// Create a map
	gameData.gameMap.create(20, 20);

	// Add blocks to the map
	for (int y = 0; y < gameData.gameMap.h; y++)
	{
		for (int x = 0; x < gameData.gameMap.w; x++)
		{
			// Make a border around the map
			if (x == 0 || x == gameData.gameMap.w - 1 || y == 0 || y == gameData.gameMap.h - 1)
				gameData.gameMap.getBlockUnsafe(x, y).type = Block::stone;

			// Each block will randomly pick one of the 4 textures for its type, so the map looks less repetitive
			gameData.gameMap.getBlockUnsafe(x, y).randIndex = std::rand() % 4;
			gameData.gameMap.getWallBlockUnsafe(x, y).randIndex = std::rand() % 4;
		}
	}

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

#pragma region keyboard_input
	// Camera movement: shift the target (the world point we're looking at).
	// Multiplying by deltaTime makes the speed framerate-independent.
	if (IsKeyDown(KEY_A)) { gameData.camera.target.x -= gameData.cameraSpeed * deltaTime; } // pan left
	if (IsKeyDown(KEY_D)) { gameData.camera.target.x += gameData.cameraSpeed * deltaTime; } // pan right
	if (IsKeyDown(KEY_W)) { gameData.camera.target.y -= gameData.cameraSpeed * deltaTime; } // pan up
	if (IsKeyDown(KEY_S)) { gameData.camera.target.y += gameData.cameraSpeed * deltaTime; } // pan down

	// Change the block being placed using 0-9
	static int currentBlock = Block::dirt;
	int key = GetKeyPressed();
	switch (key)
	{
		case KEY_ONE:   currentBlock = Block::furnace;        break;
		case KEY_TWO:   currentBlock = Block::ironBlock;     break;
		case KEY_THREE: currentBlock = Block::goldBlock;      break;
		case KEY_FOUR:  currentBlock = Block::sand;    break;
		case KEY_FIVE:  currentBlock = Block::grassBlock;       break;
		case KEY_SIX:   currentBlock = Block::glass;      break;
		case KEY_SEVEN: currentBlock = Block::goldBlock;       break;
		case KEY_EIGHT: currentBlock = Block::woodLog;   break;
		case KEY_NINE:  currentBlock = Block::leaves;   break;
		case KEY_ZERO:  currentBlock = Block::woodenChest; break;
	}

	// This is used to show which block is selected
	Vector2 worldPos = GetScreenToWorld2D(GetMousePosition(), gameData.camera);
	int blockX = (int)floor(worldPos.x);
	int blockY= (int)floor(worldPos.y);

	// Holding shift toggles "hover mode" which allows placing blocks on the wall layer instead of the main layer
	bool shiftDown = IsKeyDown(KEY_LEFT_SHIFT);

	if (shiftDown)
	{
		gameData.hoverMode = 1;
	}
	else
	{
		gameData.hoverMode = 0;
	}

	// Remove a block
	if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
	{
		if (shiftDown)
		{
			Block* b = gameData.gameMap.getWallBlockSafe(blockX, blockY);
			if (b)
			{
				*b = {};
			}
		}
		else
		{
			Block* b = gameData.gameMap.getBlockSafe(blockX, blockY);
			if (b)
			{
				*b = {};
			}
		}
	}
	
	// Place a block
	if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
	{
		if (shiftDown)
		{
			Block *b = gameData.gameMap.getWallBlockSafe(blockX, blockY);
			if (b)
			{
				b->type = currentBlock;
			}
		}
		else
		{
			Block* b = gameData.gameMap.getBlockSafe(blockX, blockY);
			if (b)
			{
				b->type = currentBlock;
			}
		}
	}
#pragma endregion

#pragma region rendering 

	// Change the background color
	ClearBackground({ 75, 75, 150, 255 });

	// Everything drawn between BeginMode2D and EndMode2D is rendered in world space,
	// transformed through the camera (target, offset, zoom, rotation).
	BeginMode2D(gameData.camera);

#pragma region culling
	// Convert the screen corners (pixels) into world coordinates.
	// Screen pixel (0,0) is the top-left; (screenW, screenH) is the bottom-right.
	// The result is in world units (blocks), so it can be fractional, e.g. (-9.6, -5.4).
	Vector2 topLeftView = GetScreenToWorld2D({ 0, 0 }, gameData.camera);
	Vector2 bottomRightView = GetScreenToWorld2D({ (float)GetScreenWidth(), (float)GetScreenHeight()}, gameData.camera);

	// Convert the fractional world coords to integer block indices.
	// floorf on the start rounds DOWN so we include any partially-visible block on the left/top edge.
	// ceilf on the end rounds UP so we include any partially-visible block on the right/bottom edge.
	// The -1/+1 adds one extra block of padding in case of floating-point imprecision at the edges.
	int startXView = (int)floorf(topLeftView.x - 1);
	int endXView = (int)ceilf(bottomRightView.x + 1);
	int startYView = (int)floorf(topLeftView.y - 1);
	int endYView = (int)ceilf(bottomRightView.y + 1);

	// The camera can look at negative world space (e.g. when centered near the map origin),
	// so startX/Y can be negative and endX/Y can exceed the map size.
	// Clamp forces all four values into the valid array index range [0, w/h - 1] to prevent out-of-bounds access.
	startXView = Clamp(startXView, 0, gameData.gameMap.w - 1);
	endXView = Clamp(endXView, 0, gameData.gameMap.w - 1);
	startYView = Clamp(startYView, 0, gameData.gameMap.h - 1);
	endYView = Clamp(endYView, 0, gameData.gameMap.h - 1);
#pragma endregion

	// There are 2 rendering loops: one for wall blocks and one for main blocks.
	// This is because some block types have both a main block and a wall block texture,
	// and the wall block needs to be drawn first so the main block appears on top of it.

	// Draw the wall blocks first, so that if a block and wall block occupy the same space,
	// the block will be drawn on top of the wall block
	for (int y = startYView; y <= endYView; y++)
	{
		for (int x = startXView; x <= endXView; x++)
		{
			Block& b = gameData.gameMap.getWallBlockUnsafe(x, y);

			if (b.type != Block::air)
			{
				if (Block::wallColumn[b.type] == -1)
				{
					// This block type doesn't have a wall texture, so skip drawing it
					continue;
				}

				float size = 1; 
				Texture2D textureAtlas = assetManager.textures;
				// Use the wallColumn array to lookup the correct column in the texture atlas for the wall type of this block
				// randIndex is used to add some variation so not all wall blocks of the same type look identical
				Rectangle textureAtlasRect = getTextureAtlas(Block::wallColumn[b.type], b.randIndex, 32, 32);


				// Draw the block
				DrawTexturePro(
					textureAtlas,
					textureAtlasRect,
					{ float(x), float(y), size, size },
					{ 0, 0 },
					0.0f,
					WHITE
				);
			}
		}
	}



	// Now draw the main blocks on top of the wall blocks
	for (int y = startYView; y <= endYView; y++)
	{
		for (int x = startXView; x <= endXView; x++)
		{
			// Get the current block
			Block& b = gameData.gameMap.getBlockUnsafe(x, y);

			if (b.type != Block::air)
			{
				// Set block properties
				float size = 1; // 1 world unit per block; zoom scales this to 100x100 pixels on screen

				Texture2D textureAtlas = assetManager.textures;
				Rectangle textureAtlasRect = getTextureAtlas(b.type, b.randIndex, 32, 32);

				// Special handling for wood logs: they have different textures based on adjacent leaves
				if (b.type == Block::woodLog)
				{
					textureAtlas = assetManager.woodLogs;

					bool between_leaves = (gameData.gameMap.getBlockUnsafe(x - 1, y).type == Block::leaves &&
						                   gameData.gameMap.getBlockUnsafe(x + 1, y).type == Block::leaves);

					bool right_leaves =    gameData.gameMap.getBlockUnsafe(x + 1, y).type == Block::leaves;
					bool left_leaves  =    gameData.gameMap.getBlockUnsafe(x - 1, y).type == Block::leaves;
					bool top_leaves   =    gameData.gameMap.getBlockUnsafe(x, y - 1).type == Block::leaves;

					if (top_leaves)
					{
						textureAtlasRect = getTextureAtlas(5, b.randIndex, 32, 32);
					}
					else if (between_leaves)
					{
						textureAtlasRect = getTextureAtlas(1, b.randIndex, 32, 32);
					}
					else if (right_leaves)
					{
						textureAtlasRect = getTextureAtlas(2, b.randIndex, 32, 32);
					}
					else if (left_leaves)
					{
						textureAtlasRect = getTextureAtlas(3, b.randIndex, 32, 32);
					}
					else
					{
						textureAtlasRect = getTextureAtlas(0, b.randIndex, 32, 32);
					}
				}

				// Draw the block
				DrawTexturePro(
					textureAtlas,					        // The whole texture atlas
					textureAtlasRect,		                // This is the 32x32 region to read from in the texture atlas
					{ float(x), float(y), size, size },     // This is where we draw it on screen
					{ 0, 0 },
					0.0f,
					WHITE
				);
			}
		}
	}

	// Draw the block selection frame
	DrawTexturePro(
		assetManager.frame,
		{ 0, 0, (float)assetManager.frame.width, (float)assetManager.frame.height },
		{ (float)blockX, (float)blockY, 1, 1 },
		{ 0, 0 },
		0.0f,
		gameData.hoverMode == 0 ? WHITE : RED // white frame for block layer, red frame for wall layer
	);

	// Anything drawn after this (e.g. HUD) uses raw screen coordinates, unaffected by the camera.
	EndMode2D();

	DrawFPS(10, 10); // FPS counter

#pragma endregion

	return true;
}

void closeGame()
{
	std::ofstream f(RESOURCES_PATH "f.txt");
	f << "CLOSED\n";
	f.close();
}