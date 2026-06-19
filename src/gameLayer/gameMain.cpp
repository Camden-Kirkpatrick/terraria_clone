#include "gameMain.hpp"
#include "assetManager.hpp"
#include "gameMap.hpp"
#include "helpers.hpp"
#include "blocks.hpp"
#include "worldGenerator.hpp"
#include "structure.hpp"
#include "saveMap.hpp"
#include <raylib.h>
#include <raymath.h>
#include <imgui.h>
#include <fstream>
#include <iostream>

#define MIN_CAM_ZOOM 10.0f
#define MAX_CAM_ZOOM 500.0f
#define DEFAULT_CAM_ZOOM 25.0f
#define INC_CAM_ZOOM 0.25f
#define MIN_CAM_SPEED 1.0f
#define MAX_CAM_SPEED 1000.0f
#define DEFAULT_CAM_SPEED 100.0f
#define INC_CAM_SPEED 0.333f

struct GameData
{ 
	GameMap gameMap = {};
	Camera2D camera = {};
	float cameraSpeed = 0.0f;
	enum HoverMode
	{
		blockLayer = 0,
		wallLayer = 1,
	} hoverMode = blockLayer; // Determines whether we are placing/breaking normal blocks or wall blocks
	int currentBlock = Block::dirt;
	int currentBlockVariant = 0; // Random variation for the current block
	// Used to place a grid of blocks (x=3, y=2 = 3x2 grid of blocks)
	struct BlockShape
	{
		int x = 1;
		int y = 1;
	} blockShape;
	Structure copyStructure = {}; // A Structure is a grid of blocks (similar to a map) that can be saved and loaded to/from a file
	Vector2 selectionStart = {}; // Beginning of the area to be copied
	Vector2 selectionEnd = {}; // End of the area to be copied
	char structureFile[100] = {}; // Name of the file the structure is saved to/loaded from
	bool previewStructure = true; // Show a preview of the structure before it's placed
} gameData;

AssetManager assetManager; // Global asset manager instance to load and store textures

// Toggle the ImGui menu on/off
// Blocks can't be broken or placed behind the ImGui menu
bool showImGui = true;
bool showAdvancedSettings = false; // Hide or show certain settings

bool initGame(bool resetWorldGen, bool resetCamera)
{
	// Load assets (textures)
	assetManager.loadAll();

	// Generate the world
	generateWorld(gameData.gameMap, worldWidth, worldHeight, seed, resetWorldGen);

	// Camera setup
	if (resetCamera)
	{
		gameData.camera.target = { (float)worldWidth / 2, 325 };       // The world-space point the camera looks at
		gameData.camera.rotation = 0.0f;							   // No rotation
		gameData.camera.zoom = DEFAULT_CAM_ZOOM;
		gameData.cameraSpeed = DEFAULT_CAM_SPEED;
	}
	
	return true;
}

bool updateGame()
{
	float deltaTime = GetFrameTime();
	if (deltaTime > 0.05f) deltaTime = 0.05f; // Clamp to 20fps minimum

	// Offset is the screen-space pixel that the target maps to.
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
	 
	// Camera zoom
	if (IsKeyDown(KEY_MINUS)) { gameData.camera.zoom -= INC_CAM_ZOOM; }
	if (IsKeyDown(KEY_EQUAL)) { gameData.camera.zoom += INC_CAM_ZOOM; }
	// Clamp values to within the valid range
	if (gameData.camera.zoom > MAX_CAM_ZOOM) { gameData.camera.zoom = MAX_CAM_ZOOM; }
	else if (gameData.camera.zoom < MIN_CAM_ZOOM) { gameData.camera.zoom = MIN_CAM_ZOOM; }

	// Camera speed
	if (IsKeyDown(KEY_LEFT_BRACKET)) { gameData.cameraSpeed -= INC_CAM_SPEED; }
	if (IsKeyDown(KEY_RIGHT_BRACKET)) { gameData.cameraSpeed += INC_CAM_SPEED; }
	// Clamp values to within the valid range
	if (gameData.cameraSpeed > MAX_CAM_SPEED) { gameData.cameraSpeed = MAX_CAM_SPEED; }
	else if (gameData.cameraSpeed < MIN_CAM_SPEED) { gameData.cameraSpeed = MIN_CAM_SPEED; }


	// This is used to show which block the mouse is hovered on (selection frame)
	Vector2 worldPos = GetScreenToWorld2D(GetMousePosition(), gameData.camera);
	int blockX = (int)floor(worldPos.x);
	int blockY = (int)floor(worldPos.y);

	// Select the block the mouse/selection frame is hovering over
	if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE))
	{
		Block* b = gameData.gameMap.getBlockSafe(blockX, blockY);
		gameData.currentBlock = b->type;
		gameData.currentBlockVariant = b->randIndex;
	}

	int key = GetKeyPressed();
	switch (key)
	{
		case KEY_TAB: showImGui = !showImGui; break;

		case KEY_R:	   
			if (!showImGui) // prevent game inputs when typing in the text boxes
				initGame(true, false); break;
	}

	// Select an area to copy and paste blocks, or save and load blocks to/from a file
	if (showImGui)
	{
		if (IsKeyPressed(KEY_ONE))
			gameData.selectionStart = Vector2{ (float)blockX, (float)blockY };
		if (IsKeyPressed(KEY_TWO))
			gameData.selectionEnd = Vector2{ (float)blockX, (float)blockY };

		// Copy the selected area
		if (IsKeyDown(KEY_LEFT_CONTROL))
		{
			if (IsKeyDown(KEY_C))
			{
				gameData.copyStructure.copyFromMap(
					gameData.gameMap,
					gameData.selectionStart,
					gameData.selectionEnd
				);
			}
		}
		// Paste the selected area
		if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
		{
			gameData.copyStructure.pasteIntoMap(
				gameData.gameMap,
				Vector2{ (float)blockX, (float)blockY }
			);
		}

		// Ensure selectionStart is before selectionEnd
		if (gameData.selectionStart.x > gameData.selectionEnd.x)
			std::swap(gameData.selectionStart.x, gameData.selectionEnd.x);
		if (gameData.selectionStart.y > gameData.selectionEnd.y)
			std::swap(gameData.selectionStart.y, gameData.selectionEnd.y);
	}


	// Holding shift toggles "hover mode" which allows placing blocks on the wall layer instead of the main layer
	bool shiftDown = IsKeyDown(KEY_LEFT_SHIFT);

	if (shiftDown)
		gameData.hoverMode = GameData::HoverMode::wallLayer;
	else
		gameData.hoverMode = GameData::HoverMode::blockLayer;

	// Don't allow placing and breaking blocks while the ImGui menu is shown
	if (!showImGui)
	{
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
					// More than one block may be placed depending on the blockShape
					for (int x = 0; x < gameData.blockShape.x; x++)
					{
						for (int y = 0; y < gameData.blockShape.y; y++)
						{
							Block* b = gameData.gameMap.getWallBlockSafe(blockX + x, blockY + y);
							if (b)
							{
								b->type = gameData.currentBlock;
								b->randIndex = std::rand() % 4; // Pick a random texture when placing a block
							}
						}
					}
				}
				else
				{
					for (int x = 0; x < gameData.blockShape.x; x++)
					{
						for (int y = 0; y < gameData.blockShape.y; y++)
						{
							Block* b = gameData.gameMap.getBlockSafe(blockX + x, blockY + y);
							if (b)
							{
								b->type = gameData.currentBlock;
								b->randIndex = gameData.currentBlockVariant;

								if (gameData.currentBlock == Block::door) 
									gameData.currentBlockVariant = std::rand() % 2; // doors only have 2 variations
								else
									gameData.currentBlockVariant = std::rand() % 4;
							}
						}
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

			if (b.type == Block::air)
				continue;

			if (Block::wallColumn[b.type] == -1)
				// This block type doesn't have a wall texture, so skip drawing it
				continue;

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



	// Now draw the main blocks on top of the wall blocks
	for (int y = startYView; y <= endYView; y++)
	{
		for (int x = startXView; x <= endXView; x++)
		{
			// Get the current block
			Block& b = gameData.gameMap.getBlockUnsafe(x, y);

			if (b.type == Block::air)
				continue;

			// Set block properties
			float size = 1.0f; // 1 world unit per block; zoom scales this to 100x100 pixels on screen

			// How tall to draw this block on screen, in world units.
			// Most blocks are 1 tall, but the door's texture is 32x64 (twice as tall),
			// so it must be drawn 2 world units tall to keep its aspect ratio.
			float drawHeight = size;

			Texture2D textureAtlas = assetManager.textures;
			Rectangle textureAtlasRect;
			if (b.type == 31)
			{
				textureAtlasRect = getTextureAtlas(b.type, b.randIndex, 32, 64);
				drawHeight = 2.0f * size;
			}
			else
				textureAtlasRect = getTextureAtlas(b.type, b.randIndex, 32, 32);

#pragma region wood_log_leaves
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
#pragma endregion

			// Draw the block
			DrawTexturePro(
				textureAtlas,					            // The whole texture atlas
				textureAtlasRect,		                    // The region to read from in the texture atlas
				{ float(x), float(y), size, drawHeight },   // This is where we draw it on screen
				{ 0, 0 },
				0.0f,
				WHITE
			);
		}
	}

	// If a door is the block currently selected, make sure to get a 32x64 area instead of a 32x32 area
	float blockHeight = 1;
	Rectangle textureAtlasRect = getTextureAtlas(gameData.currentBlock, gameData.currentBlockVariant, 32, 32);
	if (gameData.currentBlock == Block::door)
	{
		textureAtlasRect = getTextureAtlas(gameData.currentBlock, gameData.currentBlockVariant, 32, 64);
		blockHeight = 2;
	}
	
	// Show a prevew of the block currently selected
	if (!showImGui)
	{
		DrawTexturePro(
			assetManager.textures,
			textureAtlasRect,
			{ (float)blockX, (float)blockY, 1, blockHeight },
			{ 0, 0 },
			0.0f,
			{ 255, 255, 255, 255 }
		);
	}

	// Show a preview of the current structure that was copied or loaded from a file
	if (gameData.previewStructure && showImGui)
	{
		// Show the walls in the preview
		for (int x = 0; x < gameData.copyStructure.w; x++)
		{
			for (int y = 0; y < gameData.copyStructure.h; y++)
			{
				Block& b = gameData.copyStructure.getWallBlockUnsafe(x, y);

				if (b.type == Block::air)
					continue;

				if (Block::wallColumn[b.type] == -1)
					continue;

					DrawTexturePro(
						assetManager.textures,
						getTextureAtlas(Block::wallColumn[b.type], b.randIndex, 32, 32),
						{ (float)blockX + x, (float)blockY + y, 1, 1 },
						{ 0, 0 },
						0.0f,
						{ 255, 255, 255, 175 }
					);
			}
		}

		// Show the blocks in the preview
		for (int x = 0; x < gameData.copyStructure.w; x++)
		{
			for (int y = 0; y < gameData.copyStructure.h; y++)
			{
				Block& b = gameData.copyStructure.getBlockUnsafe(x, y);

				if (b.type == Block::air)
					continue;

				float blockHeight = 1;
				Rectangle textureAtlasRect = getTextureAtlas(b.type, b.randIndex, 32, 32);
				if (b.type == Block::door)
				{
					textureAtlasRect = getTextureAtlas(b.type, b.randIndex, 32, 64);
					blockHeight = 2;
				}
					
				DrawTexturePro(
					assetManager.textures,
					textureAtlasRect,
					{ (float)blockX + x, (float)blockY + y, 1, blockHeight },
					{ 0, 0 },
					0.0f,
					{ 255, 255, 255, 175 }
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

	// Draw the selection rectangle used for copying and pasting blocks
	if (showImGui)
	{
		Rectangle rect;
		rect.x = gameData.selectionStart.x;
		rect.y = gameData.selectionStart.y; 
		// Size = end - start + 1
		rect.width = (gameData.selectionEnd.x - gameData.selectionStart.x) + 1;
		rect.height = (gameData.selectionEnd.y - gameData.selectionStart.y) + 1;

		DrawRectangleLinesEx(rect, 0.1f, { 20, 101, 250, 145 });
	}


	// Anything drawn after this (e.g. HUD) uses raw screen coordinates, unaffected by the camera.
	EndMode2D();

#pragma region game_menu
#if PRODUCTION_BUILD == 0 || MENU
	if (showImGui)
	{
		ImGui::Begin("Game Menu");
		ImGui::Text("Press 'TAB' to open/close the menu");
		ImGui::Text("Menu must be closed in order to place/break blocks");
		ImGui::Text("Press 'r' to reset world-gen settings to the default");

		//int cameraX = (int)gameData.camera.target.x;
		//if (cameraX >= 0 && cameraX < (int)savedBiomeNoise.size())
		//{
		//	float bn = savedBiomeNoise[cameraX];
		//	const char* branch =
		//		(bn > worldGen.plainThreshold - worldGen.terrainBlendZone &&
		//			bn < worldGen.plainThreshold + worldGen.terrainBlendZone) ? "BLEND" :
		//		(bn < worldGen.plainThreshold) ? "PLAINS" : "MOUNTAINS";
		//	ImGui::Text("Column %d  biomeNoise=%.5f  branch=%s", cameraX, bn, branch);
		//}

		ImGui::Text("Camera zoom:");  ImGui::SameLine(); ImGui::SliderFloat("##camZoom", &gameData.camera.zoom, MIN_CAM_ZOOM, MAX_CAM_ZOOM);
		ImGui::Text("Camera speed:"); ImGui::SameLine(); ImGui::SliderFloat("##camSpeed", &gameData.cameraSpeed, MIN_CAM_SPEED, MAX_CAM_SPEED);
		ImGui::Separator();



		ImGui::Text("World width"); ImGui::SameLine();
		if (ImGui::SliderInt("##worldWidth", &worldWidth, 25, 10000))
			initGame(false, false);
		ImGui::Text("World height"); ImGui::SameLine();
		if (ImGui::SliderInt("##worldHeight", &worldHeight, 375, 1000))
			initGame(false, false);
		ImGui::Separator();

		ImGui::Text("Seed:"); ImGui::SameLine();
		if (ImGui::InputInt("##seed", &seed))
			initGame(false, false);
		ImGui::Separator();



		if (ImGui::Button("Reset world generation settings"))
		{
			initGame(true, false);
		}

		if (ImGui::Button("Generate flat world"))
		{
			flatWorld();
			initGame(false, false);
		}

		if (ImGui::Button("Go to world spawn"))
		{
			initGame(false, true);
		}

		if (ImGui::Checkbox("Show advanced settings", &showAdvancedSettings)) {}

		ImGui::Separator();



		if (showAdvancedSettings)
		{
			ImGui::Text("World Generation Settings");

			ImGui::Text("Mountain Settings");
			//ImGui::Text("Dirt mountain octaves:");   ImGui::SameLine(); ImGui::SliderInt("##dirtMtnOct", &worldGen.dirtMountainOctaves, 1, 20);
			ImGui::Text("Dirt mountain frequency:"); ImGui::SameLine();
			if (ImGui::SliderFloat("##dirtMtnFreq", &worldGen.dirtMountainFrequency, 0.00001f, 0.1f, "%.5f"))
				initGame(false, false);

			ImGui::Text("Min dirt mountain thickness:"); ImGui::SameLine();
			if (ImGui::SliderInt("##minDirtMtnThick", &worldGen.minDirtMountainThickness, 1, 50))
				initGame(false, false);
			ImGui::Text("Max dirt mountain thickness:"); ImGui::SameLine();
			if (ImGui::SliderInt("##maxDirtMtnThick", &worldGen.maxDirtMountainThickness, 1, 50))
				initGame(false, false);

			//ImGui::Text("Stone mountain octaves:");  ImGui::SameLine(); ImGui::SliderInt("##stnMtnOct", &worldGen.stoneMountainOctaves, 1, 20);
			ImGui::Text("Stone mountain frequency:"); ImGui::SameLine();
			if (ImGui::SliderFloat("##stnMtnFreq", &worldGen.stoneMountainFrequency, 0.00001f, 0.1f, "%.5f"))
				initGame(false, false);
			ImGui::Text("Min stone mountain start:"); ImGui::SameLine();
			if (ImGui::SliderInt("##minStoneMtnStart", &worldGen.minStoneMountainStart, 330, 400))
				initGame(false, false);
			ImGui::Text("Max stone mountain start:"); ImGui::SameLine();
			if (ImGui::SliderInt("##maxStoneMtnStart", &worldGen.maxStoneMountainStart, 330, 400))
				initGame(false, false);
			ImGui::Separator();



			ImGui::Text("Plains Settings");
			//ImGui::Text("Dirt plain octaves:");   ImGui::SameLine(); ImGui::SliderInt("##dirtPlnOct", &worldGen.dirtPlainOctaves, 1, 20);
			ImGui::Text("Dirt plain frequency:"); ImGui::SameLine();
			if (ImGui::SliderFloat("##dirtPlnFreq", &worldGen.dirtPlainFrequency, 0.00001f, 0.1f, "%.5f"))
				initGame(false, false);

			ImGui::Text("Min dirt plain thickness:"); ImGui::SameLine();
			if (ImGui::SliderInt("##minDirtPlnThick", &worldGen.minDirtPlainThickness, 1, 50))
				initGame(false, false);
			ImGui::Text("Max dirt plain thickness:"); ImGui::SameLine();
			if (ImGui::SliderInt("##maxDirtPlnThick", &worldGen.maxDirtPlainThickness, 1, 50))
				initGame(false, false);

			//ImGui::Text("Stone plain octaves:");  ImGui::SameLine(); ImGui::SliderInt("##stnPlnOct", &worldGen.stonePlainOctaves, 1, 20);
			ImGui::Text("Stone plain frequency:"); ImGui::SameLine();
			if (ImGui::SliderFloat("##stnPlnFreq", &worldGen.stonePlainFrequency, 0.00001f, 0.1f, "%.5f"))
				initGame(false, false);

			ImGui::Text("Min stone plain start:"); ImGui::SameLine();
			if (ImGui::SliderInt("##minStonePlnStart", &worldGen.minStonePlainStart, 330, 400))
				initGame(false, false);
			ImGui::Text("Max stone plain start:"); ImGui::SameLine();
			if (ImGui::SliderInt("##maxStonePlnStart", &worldGen.maxStonePlainStart, 330, 400))
				initGame(false, false);
			ImGui::Separator();



			ImGui::Text("Terrain and Biome Settings");

			//ImGui::Text("Terrain octaves:");  ImGui::SameLine(); ImGui::SliderInt("##terrOct", &worldGen.terrainOctaves, 1, 20);
			ImGui::Text("Terrain frequency:"); ImGui::SameLine();
			if (ImGui::SliderFloat("##terrFreq", &worldGen.terrainFrequency, 0.00001f, 0.01f, "%.5f"))
				initGame(false, false);
			ImGui::Text("Terrain blend zone:"); ImGui::SameLine();
			if (ImGui::SliderFloat("##terrBlendZone", &worldGen.terrainBlendZone, 0.0f, 1.0f, "%.5f"))
				initGame(false, false);

			ImGui::Text("Min desert threshold:"); ImGui::SameLine();
			if (ImGui::SliderFloat("##minDesThresh", &worldGen.minDesertThreshold, 0.0f, 1.0f, "%.5f"))
				initGame(false, false);
			ImGui::Text("Max desert threshold:"); ImGui::SameLine();
			if (ImGui::SliderFloat("##maxDesThresh", &worldGen.maxDesertThreshold, 0.0f, 1.0f, "%.5f"))
				initGame(false, false);

			//ImGui::Text("Desert octaves:");  ImGui::SameLine(); ImGui::SliderInt("##desOct", &worldGen.desertOctaves, 1, 20);
			ImGui::Text("Desert frequency:"); ImGui::SameLine();
			if (ImGui::SliderFloat("##desFreq", &worldGen.desertFrequency, 0.00001f, 0.1f, "%.5f"))
				initGame(false, false);

			ImGui::Text("Desert blend zone:"); ImGui::SameLine();
			if (ImGui::SliderFloat("##desBlendZone", &worldGen.desertBlendZone, 0.0f, 1.0f, "%.5f"))
				initGame(false, false);
			ImGui::Separator();



			ImGui::Text("Cave Settings");
			if (ImGui::Checkbox("Generate caves", &worldGen.generateCaves))
			{
				initGame(false, false);
			}

			//ImGui::Text("Cave octaves:");  ImGui::SameLine(); ImGui::SliderInt("##caveOctOct", &worldGen.caveOctaves, 1, 20);
			ImGui::Text("Cave frequency:"); ImGui::SameLine();
			if (ImGui::SliderFloat("##caveFreq", &worldGen.caveFrequency, 0.00001f, 0.1f, "%.5f"))
				initGame(false, false);

			ImGui::Text("Min cave threshold:"); ImGui::SameLine();
			if (ImGui::SliderFloat("##minCaveThresh", &worldGen.minCaveThreshold, 0.0f, 1.0f, "%.5f"))
				initGame(false, false);
			ImGui::Text("Max cave threshold:"); ImGui::SameLine();
			if (ImGui::SliderFloat("##maxCaveThresh", &worldGen.maxCaveThreshold, 0.0f, 1.0f, "%.5f"))
				initGame(false, false);
			ImGui::Separator();



			ImGui::Text("Tunnel Settings");
			if (ImGui::Checkbox("Generate tunnels", &worldGen.generateWorms))
			{
				initGame(false, false);
			}

			ImGui::Text("Current number of tunnels: %d", worldGen.curNumWorms);

			ImGui::Text("Min tunnel length:"); ImGui::SameLine();
			if (ImGui::SliderInt("##minTunlLen", &worldGen.minWormLength, 50, 500))
			{
				// min/maxWormWidth use getRandomInt(), which requires the first arg to be greater than or equal to the second arg
				if (worldGen.minWormLength > worldGen.maxWormLength)
					worldGen.maxWormLength = worldGen.minWormLength;
				initGame(false, false);
			}
			ImGui::Text("Max tunnel width:"); ImGui::SameLine();
			if (ImGui::SliderInt("##maxTunlLen", &worldGen.maxWormLength, 50, 500))
			{
				if (worldGen.maxWormLength < worldGen.minWormLength)
					worldGen.minWormLength = worldGen.maxWormLength;
				initGame(false, false);
			}

			ImGui::Text("Min tunnel width:"); ImGui::SameLine();
			if (ImGui::SliderInt("##minTunlWidth", &worldGen.minWormWidth, 1, 20))
			{
				// min/maxWormWidth use getRandomInt(), which requires the first arg to be greater than or equal to the second arg
				if (worldGen.minWormWidth > worldGen.maxWormWidth)
					worldGen.maxWormWidth = worldGen.minWormWidth;
				initGame(false, false);
			}
			ImGui::Text("Max tunnel width:"); ImGui::SameLine();
			if (ImGui::SliderInt("##maxTunlWidth", &worldGen.maxWormWidth, 1, 20))
			{
				if (worldGen.maxWormWidth < worldGen.minWormWidth)
					worldGen.minWormWidth = worldGen.maxWormWidth;
				initGame(false, false);
			}

			ImGui::Text("Min tunnel turn angle:"); ImGui::SameLine();
			if (ImGui::SliderFloat("##minTunlAngle", &worldGen.minWormTurnAngle, -0.2f, 0.2f))
			{
				// min/maxWormTurnAngle use getRandomInt(), which requires the first arg to be greater than or equal to the second arg
				if (worldGen.minWormTurnAngle > worldGen.maxWormTurnAngle)
					worldGen.maxWormTurnAngle = worldGen.minWormTurnAngle;
				initGame(false, false);
			}
			ImGui::Text("Max tunnel turn angle:"); ImGui::SameLine();
			if (ImGui::SliderFloat("##maxTunlAngle", &worldGen.maxWormTurnAngle, -0.2f, 0.2f))
			{
				if (worldGen.maxWormTurnAngle < worldGen.minWormTurnAngle)
					worldGen.minWormTurnAngle = worldGen.maxWormTurnAngle;
				initGame(false, false);
			}
			ImGui::Separator();


			ImGui::Text("Special Material Settings");
			ImGui::Text("Ore threshold:"); ImGui::SameLine();
			if (ImGui::SliderInt("##oreThresh", &worldGen.oreThreshold, 0, 499))
				initGame(false, false);
			ImGui::Text("Gold chance:"); ImGui::SameLine();
			if (ImGui::SliderFloat("##gldChance", &worldGen.goldChance, 0.0f, 1.0f, "%.5f"))
				initGame(false, false);
			ImGui::Text("Iron chance:"); ImGui::SameLine();
			if (ImGui::SliderFloat("##irnChance", &worldGen.ironChance, 0.0f, 1.0f, "%.5f"))
				initGame(false, false);
			ImGui::Text("Copper chance:"); ImGui::SameLine();
			if (ImGui::SliderFloat("##copChance", &worldGen.copperChance, 0.0f, 1.0f, "%.5f"))
				initGame(false, false);

			ImGui::Text("Ruby threshold:"); ImGui::SameLine();
			if (ImGui::SliderInt("##rubThresh", &worldGen.rubyThreshold, 0, 499))
				initGame(false, false);
			ImGui::Text("Ruby chance:"); ImGui::SameLine();
			if (ImGui::SliderFloat("##rubChance", &worldGen.rubyChance, 0.0f, 1.0f, "%.5f"))
				initGame(false, false);

			ImGui::Text("Clay threshold:"); ImGui::SameLine();
			if (ImGui::SliderInt("##clyThresh", &worldGen.clayThreshold, 0, 499))
				initGame(false, false);
			ImGui::Text("Clay chance:"); ImGui::SameLine();
			if (ImGui::SliderFloat("##clyChance", &worldGen.clayChance, 0.0f, 1.0f, "%.5f"))
				initGame(false, false);
		}



		ImGui::End();



		ImGui::Begin("Block Selection");
		ImGui::Text("Press F1 to open/close the menu");
		ImGui::Text("Use middle click on your mouse to select the block being hovered over");

		ImGui::Text("Press '1' to change the start of the selection area");
		ImGui::Text("Press '2' to change the end of the selection area");
		ImGui::Text("Press 'Left CTRL + c' to copy the selected area");
		ImGui::Text("Press 'Mouse Right Click' to paste the copied area");

		ImGui::InputText("File name", gameData.structureFile, sizeof(gameData.structureFile));

		if (ImGui::Button("Save to file"))
		{
			std::string path = RESOURCES_PATH "structures/";
			path += gameData.structureFile;
			path += ".bin";

			saveBlockDataToFile(
				gameData.copyStructure.structureBlocks,
				gameData.copyStructure.structureWallBlocks,
				gameData.copyStructure.w,
				gameData.copyStructure.h,
				path.c_str()
			);
		}

		if (ImGui::Button("Load from file"))
		{
			std::string path = RESOURCES_PATH "structures/";
			path += gameData.structureFile;
			path += ".bin";

			loadBlockDataFromFile(
				gameData.copyStructure.structureBlocks,
				gameData.copyStructure.structureWallBlocks,
				gameData.copyStructure.w,
				gameData.copyStructure.h,
				path.c_str()
			);
		}

		if (ImGui::Button("Copy World"))
		{
			gameData.selectionStart.x = 0;
			gameData.selectionStart.y = 0;
			gameData.selectionEnd.x = worldWidth - 1;
			gameData.selectionEnd.y = worldHeight - 1;

			gameData.copyStructure.copyFromMap(
				gameData.gameMap,
				gameData.selectionStart,
				gameData.selectionEnd
			);
		}

		ImGui::Separator();

		if (ImGui::Checkbox("Preview structure", &gameData.previewStructure)) {}

		ImGui::Text("Change the block shape to a 'm x n' grid:");
		if (ImGui::SliderInt("##blckShpX", &gameData.blockShape.x, 1, 100)) {}
		if (ImGui::SliderInt("##blckShpY", &gameData.blockShape.y, 1, 100)) {}

		ImGui::Separator();

		ImGui::Text("Air"); ImGui::SameLine();

		// Loop over every block type, skipping air (type 0).
		// i doubles as the block ID and the atlas column index.
		for (uint16_t i = 0; i < Block::BLOCKS_COUNT; i++)
		{
			// Get the tile's pixel rect in the atlas (column i, row 0, 32x32).
			Rectangle atlas = getTextureAtlas(i, 0, 32, 32);

			if (i == Block::door)
				atlas = getTextureAtlas(i, 0, 32, 64);

			// Convert pixel coords to UVs (0..1) - what ImageButton expects.
			atlas.x /= assetManager.textures.width;
			atlas.width /= assetManager.textures.width;
			atlas.y /= assetManager.textures.height;
			atlas.height /= assetManager.textures.height;

			// ImageButton has no label, so every button would share the same ID
			// and collide on hover/click state. Push i to make each one unique.
			ImGui::PushID(i);

			// ImTextureID is an opaque void*; raylib/OpenGL stores the GL texture
			// ID inside it. The intptr_t hop avoids a 64-bit int-to-pointer warning.
			ImTextureID tex = (ImTextureID)(intptr_t)assetManager.textures.id;

			// 35x35 button showing the atlas sub-region from uv0 to uv1.
			float blockHeight = 32;
			if (i == Block::door)
				blockHeight = 64;

			if (ImGui::ImageButton(tex,
				{ 32, blockHeight }, { atlas.x, atlas.y },
				{ atlas.x + atlas.width, atlas.y + atlas.height }))
			{
				gameData.currentBlock = i;
			}

			ImGui::PopID();

			// 10 buttons per row: SameLine keeps the next widget inline; skipping
			// it every 10th iteration drops to a new row.
			if (i % 10 != 0)
			{
				ImGui::SameLine();
			}
		}

		ImGui::End();
	}
#endif
#pragma endregion

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