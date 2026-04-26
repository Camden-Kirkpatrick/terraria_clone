#include "gameMain.hpp"
#include "assetManager.hpp"
#include "gameMap.hpp"
#include "helpers.hpp"
#include "blocks.hpp"
#include "worldGenerator.hpp"
#include <raylib.h>
#include <raymath.h>
#include <imgui.h>
#include <rlImGui.h>
#include <fstream>
#include <iostream>

#define MIN_CAM_ZOOM 10.0f
#define MAX_CAM_ZOOM 200.0f
#define DEFAULT_CAM_ZOOM 100.0f
#define MIN_CAM_SPEED 1.0f
#define MAX_CAM_SPEED 100.0f
#define DEFAULT_CAM_SPEED 15.0f

struct GameData
{ 
	GameMap gameMap;
	Camera2D camera = {};
	float cameraSpeed = 0.0f;
	enum HoverMode
	{
		blockLayer = 0,
		wallLayer = 1,
	} hoverMode = blockLayer; // determines whether we are placing/breaking normal blocks or wall blocks
} gameData;

AssetManager assetManager; // global asset manager instance to load and store textures

bool initGame()
{
	// Load assets (textures)
	assetManager.loadAll();

	// Generate the world
	generateWorld(gameData.gameMap);

	// Camera setup
	gameData.camera.target = { 50, 50 };        // the world-space point the camera looks at
	gameData.camera.rotation = 0.0f;            // no rotation
	gameData.camera.zoom = DEFAULT_CAM_ZOOM;    // 1 world unit = 100 screen pixels
	gameData.cameraSpeed = DEFAULT_CAM_SPEED;
	
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
		case KEY_ONE:   currentBlock = Block::dirt;        break;
		case KEY_TWO:   currentBlock = Block::grassBlock;  break;
		case KEY_THREE: currentBlock = Block::stone;       break;
		case KEY_FOUR:  currentBlock = Block::bricks;      break;
		case KEY_FIVE:  currentBlock = Block::sand;        break;
		case KEY_SIX:   currentBlock = Block::glass;       break;
		case KEY_SEVEN: currentBlock = Block::goldBlock;   break;
		case KEY_EIGHT: currentBlock = Block::woodLog;     break;
		case KEY_NINE:  currentBlock = Block::leaves;      break;
		case KEY_ZERO:  currentBlock = Block::woodenChest; break;
		case KEY_R:     initGame(); break;
	}

	// This is used to show which block is selected
	Vector2 worldPos = GetScreenToWorld2D(GetMousePosition(), gameData.camera);
	int blockX = (int)floor(worldPos.x);
	int blockY= (int)floor(worldPos.y);

	// Holding shift toggles "hover mode" which allows placing blocks on the wall layer instead of the main layer
	bool shiftDown = IsKeyDown(KEY_LEFT_SHIFT);

	if (shiftDown)
	{
		gameData.hoverMode = GameData::HoverMode::wallLayer;
	}
	else
	{
		gameData.hoverMode = GameData::HoverMode::blockLayer;
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

	// Track the x and y of the block previously placed
	static int lastPlacedX = -1;
	static int lastPlacedY = -1;
	
	// Place a block
	if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
	{
		// If the cursor is on a new block, place it with a fresh random texture
		if (blockX != lastPlacedX || blockY != lastPlacedY)
		{
			lastPlacedX = blockX;
			lastPlacedY = blockY;

			if (shiftDown)
			{
				Block* b = gameData.gameMap.getWallBlockSafe(blockX, blockY);
				if (b)
				{
					b->type = currentBlock;
					b->randIndex = std::rand() % 4; // Pick a random texture when placing a block
				}
			}
			else
			{
				Block* b = gameData.gameMap.getBlockSafe(blockX, blockY);
				if (b)
				{
					b->type = currentBlock;
					b->randIndex = std::rand() % 4;
				}
			}
		}
	}

	// Reset so re-clicking the same tile generates a new randIndex
	if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT))
	{
		lastPlacedX = -1;
		lastPlacedY = -1;
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


				// Draw the wall block
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
					textureAtlas = assetManager.woodLogs; // wood logs have a separate texture atlas from the other blocks

					bool stump =          (gameData.gameMap.getBlockType(x, y + 1) != Block::woodLog &&
						                   gameData.gameMap.getBlockType(x, y - 1) != Block::woodLog);

					bool betweenLeaves =  (gameData.gameMap.getBlockType(x - 1, y) == Block::leaves &&
						                   gameData.gameMap.getBlockType(x + 1, y) == Block::leaves);

					bool treeBase =		  (gameData.gameMap.getBlockType(x, y + 1) != Block::woodLog &&
						                   gameData.gameMap.getBlockType(x, y - 1) == Block::woodLog);

					bool rightLeaves  =    gameData.gameMap.getBlockType(x + 1, y) == Block::leaves;
					bool leftLeaves   =    gameData.gameMap.getBlockType(x - 1, y) == Block::leaves;
					bool topLeaves    =    gameData.gameMap.getBlockType(x, y - 1) == Block::leaves;
					bool noTopLeaves  =    gameData.gameMap.getBlockType(x, y - 1) == Block::air;

					if (stump)
					{
						textureAtlasRect = getTextureAtlas(7, b.randIndex, 32, 32);
					}
					else if (topLeaves)
					{
						textureAtlasRect = getTextureAtlas(5, b.randIndex, 32, 32);
					}
					else if (noTopLeaves)
					{
						textureAtlasRect = getTextureAtlas(6, b.randIndex, 32, 32);
					}
					else if (treeBase)
					{
						textureAtlasRect = getTextureAtlas(4, b.randIndex, 32, 32);
					}
					else if (betweenLeaves)
					{
						textureAtlasRect = getTextureAtlas(1, b.randIndex, 32, 32);
					}
					else if (rightLeaves)
					{
						textureAtlasRect = getTextureAtlas(2, b.randIndex, 32, 32);
					}
					else if (leftLeaves)
					{
						textureAtlasRect = getTextureAtlas(3, b.randIndex, 32, 32);
					}
					else // normal woodLog texture
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
		gameData.hoverMode == GameData::HoverMode::blockLayer ? WHITE : RED // white frame for block layer, red frame for wall layer
	);

	// Anything drawn after this (e.g. HUD) uses raw screen coordinates, unaffected by the camera.
	EndMode2D();

	ImGui::Begin("Game control");

	ImGui::SliderFloat("Camera zoom:", &gameData.camera.zoom, MIN_CAM_ZOOM, MAX_CAM_ZOOM);
	ImGui::SliderFloat("Camera speed:", &gameData.cameraSpeed, MIN_CAM_SPEED, MAX_CAM_SPEED);

	ImGui::End();

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