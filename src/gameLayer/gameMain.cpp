#include "gameMain.hpp"
#include "assetManager.hpp"
#include "gameMap.hpp"
#include "helpers.hpp"
#include "blocks.hpp"
#include "worldGenerator.hpp"
#include "structure.hpp"
#include "saveMap.hpp"
#include "physics.hpp"
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

	int curBlock = Block::dirt; // The block currently selected to be placed
	int nextBlockVariant = 0; // Random variation for the next block

	// Used to place a grid of blocks (x=3, y=2 = 3x2 grid of blocks)
	struct BlockShape
	{
		int x = 1;
		int y = 1;
	} blockShape;

	Structure copyStructures[10] = {}; // A Structure is a grid of blocks (similar to a map) that can be saved and loaded to/from a file
	int curStructureIdx = 0; // Keep track of the structure currently selected
	Vector2 selectionStart = {-1, -1}; // Beginning of the area to be copied
	Vector2 selectionEnd = {-1, -1}; // End of the area to be copied
	char structureFile[100] = {}; // Name of the file the structure is saved to/loaded from
	bool previewStructure = true; // Show a preview of the structure before it's placed
	bool previewSelection = true; // Show the selection outline rectangle
} gameData;

AssetManager assetManager; // Global asset manager instance to load and store textures

// Toggle the ImGui menu on/off
// Blocks can't be broken or placed behind the ImGui menu
bool showImGui = true;
bool showAdvancedSettings = false; // Hide or show certain settings

// Shows a help tooltip for the most recently submitted widget when it is hovered.
static void HelpTooltip(const char* text)
{
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("%s", text);
}

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
	Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), gameData.camera);
	int blockX = (int)floor(mousePos.x);
	int blockY = (int)floor(mousePos.y);

	// Select the block the mouse/selection frame is hovering over
	if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE))
	{
		Block* b = gameData.gameMap.getBlockSafe(blockX, blockY);
		gameData.curBlock = b->type;
		gameData.nextBlockVariant = b->randIndex;
	}

	int key = GetKeyPressed();
	switch (key)
	{
		case KEY_TAB: showImGui = !showImGui; break;

		case KEY_R:	   
			if (!showImGui) // prevent game inputs when typing in the text boxes
				initGame(true, false); break;
	}

#pragma region structures
	if (showImGui)
	{
		// Change the current structure with the arrow keys
		if (IsKeyPressed(KEY_RIGHT))
		{
			if (gameData.curStructureIdx < 9)
				gameData.curStructureIdx++;
			else
				gameData.curStructureIdx = 0;
		}
		if (IsKeyPressed(KEY_LEFT))
		{
			if (gameData.curStructureIdx > 0)
				gameData.curStructureIdx--;
			else
				gameData.curStructureIdx = 9;
		}
	}

	// The structure currently selected
	Structure& curStructure = gameData.copyStructures[gameData.curStructureIdx];

	// Select an area to copy and paste blocks, or save and load blocks to/from a file
	if (showImGui)
	{
		if (IsKeyPressed(KEY_ONE))
		{
			// The first time we choose a selection area, start at the current block, instead of (0, 0)
			if (gameData.selectionStart.x == -1)
			{
				gameData.selectionStart = Vector2{ (float)blockX, (float)blockY };
				gameData.selectionEnd = Vector2{ (float)blockX, (float)blockY };
			}
			else
				gameData.selectionStart = Vector2{ (float)blockX, (float)blockY };
		}
		if (IsKeyPressed(KEY_TWO))
		{
			if (gameData.selectionEnd.x == -1)
			{
				gameData.selectionStart = Vector2{ (float)blockX, (float)blockY };
				gameData.selectionEnd = Vector2{ (float)blockX, (float)blockY };
			}
			else
				gameData.selectionEnd = Vector2{ (float)blockX, (float)blockY };
		}

		// Copy the selected area
		if (IsKeyDown(KEY_LEFT_CONTROL))
		{
			if (IsKeyDown(KEY_C))
			{
				curStructure.copyFromMap(
					gameData.gameMap,
					gameData.selectionStart,
					gameData.selectionEnd
				);
			}
		}

		// Paste the selected area
		if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
		{
			curStructure.pasteIntoMap(
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
#pragma endregion

	// Holding shift toggles "hover mode" which allows placing blocks on the wall layer instead of the main layer
	bool shiftDown = IsKeyDown(KEY_LEFT_SHIFT);

	if (shiftDown)
		gameData.hoverMode = GameData::HoverMode::wallLayer;
	else
		gameData.hoverMode = GameData::HoverMode::blockLayer;

#pragma region place_break_blocks
	// Don't allow placing and breaking blocks while the ImGui menu is shown
	if (!showImGui)
	{
		// Remove a block
		if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && blockX < worldWidth && blockY < worldHeight)
		{
			// Wall blocks
			if (shiftDown)
			{
				Block* b = gameData.gameMap.getWallBlockSafe(blockX, blockY);
				if (b)
					*b = {};
			}
			else
			{
				Block* b = gameData.gameMap.getBlockSafe(blockX, blockY);

				// ----- Special Case for Doors ----
				// Block above the current block
				Block* b0 = gameData.gameMap.getBlockSafe(blockX, blockY - 1);
				// Pressing left click on the bottom block of the door, also breaks the door
				if (b0->type == Block::door)
					*b0 = {};
				// ----------------------------------
				// Break normal block
				else
					*b = {};
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

				// Wall blocks
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
								b->type = gameData.curBlock;
								b->randIndex = std::rand() % 4; // Pick a random texture when placing a block
							}
						}
					}
				}
				// Normal blocks
				else
				{
					// ----- Special Case for Doors ----
					bool isDoor = gameData.curBlock == Block::door;
					int stopY = gameData.blockShape.y;
					int incY = 1;

					// If the current block is a door, and we want to draw an area of doors, then we have to get a 1x2 area
					if (isDoor)
					{
						stopY = gameData.blockShape.y * 2;
						incY = 2;
					}
					// ----------------------------------

					for (int x = 0; x < gameData.blockShape.x; x++)
					{
						for (int y = 0; y < stopY; y += incY)
						{
							Block* b = gameData.gameMap.getBlockSafe(blockX + x, blockY + y);
							if (b)
							{
								b->type = gameData.curBlock;
								b->randIndex = gameData.nextBlockVariant;

								if (isDoor) 
									gameData.nextBlockVariant = std::rand() % 2; // doors only have 2 variations
								else
									gameData.nextBlockVariant = std::rand() % 4;

								// Don't allow a door to be on the bottom half of another door
								Block* b0 = gameData.gameMap.getBlockSafe(blockX + x, blockY - 1 + y);
								if (b0 && b0->type == Block::door)
									*b0 = {};

								// Don't allow another block to be on the bottom half of a door
								Block *b1 = gameData.gameMap.getBlockSafe(blockX + x, blockY + 1 + y);
								if (b1 && isDoor)
									*b1 = {};
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
			if (b.type == Block::door)
			{
				textureAtlasRect = getTextureAtlas(b.type, b.randIndex, 32, 64);
				drawHeight = 2.0f * size;
			}
			else
				textureAtlasRect = getTextureAtlas(b.type, b.randIndex, 32, 32);

			// Only allow grass to be on top of grass blocks
			if (b.type == Block::grass)
			{
				if (gameData.gameMap.getBlockUnsafe(x, y + 1).type != Block::grassBlock)
					b = {};
			}

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

	// ----- Special Case for Doors ----
	// If a door is the block currently selected, make sure to get a 32x64 area instead of a 32x32 area
	float blockHeight = 1;
	Rectangle textureAtlasRect = getTextureAtlas(gameData.curBlock, gameData.nextBlockVariant, 32, 32);
	if (gameData.curBlock == Block::door)
	{
		textureAtlasRect = getTextureAtlas(gameData.curBlock, gameData.nextBlockVariant, 32, 64);
		blockHeight = 2;
	}

	int stopY = gameData.blockShape.y;
	int incY = 1;

	// Same code used for placing a grid of doors, but used for the block preview
	if (gameData.curBlock == Block::door)
	{
		stopY = gameData.blockShape.y * 2;
		incY = 2;
	}
	// ----------------------------------
	
	// Show a prevew of the block currently selected
	if (!showImGui)
	{
		for (int x = 0; x < gameData.blockShape.x; x++)
		{
			for (int y = 0; y < stopY; y += incY)
			{
				DrawTexturePro(
					assetManager.textures,
					textureAtlasRect,
					{ (float)blockX + x, (float)blockY + y, 1, blockHeight },
					{ 0, 0 },
					0.0f,
					{ 255, 255, 255, 255 }
				);
			}
		}
	}

	// Show a preview of the current structure that was copied or loaded from a file
	if (gameData.previewStructure && showImGui)
	{
		// Show the walls in the preview
		for (int x = 0; x < curStructure.w; x++)
		{
			for (int y = 0; y < curStructure.h; y++)
			{
				Block& b = curStructure.getWallBlockUnsafe(x, y);

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
		for (int x = 0; x < curStructure.w; x++)
		{
			for (int y = 0; y < curStructure.h; y++)
			{
				Block& b = curStructure.getBlockUnsafe(x, y);

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
	for (int x = 0; x < gameData.blockShape.x; x++)
	{
		for (int y = 0; y < stopY; y++)
		{
			DrawTexturePro(
				assetManager.frame,
				{ 0, 0, (float)assetManager.frame.width, (float)assetManager.frame.height },
				{ (float)blockX + x, (float)blockY + y, 1, 1 },
				{ 0, 0 },
				0.0f,
				gameData.hoverMode == GameData::HoverMode::blockLayer ? WHITE : RED // white frame for block layer, red frame for wall layer
			);
		}
	}

	// Draw the selection rectangle used for copying and pasting blocks
	if (showImGui && gameData.previewSelection)
	{
		Rectangle rect;

		rect.x = gameData.selectionStart.x;
		rect.y = gameData.selectionStart.y;
		// Size = end - start + 1
		rect.width = (gameData.selectionEnd.x - gameData.selectionStart.x) + 1;
		rect.height = (gameData.selectionEnd.y - gameData.selectionStart.y) + 1;

		DrawRectangleLinesEx(rect, 0.1f, { 20, 101, 250, 145 });
	}



	Transform2D test;
	test.pos = { 5000.5f, 335.5f };
	test.w = 1;
	test.h = 1;

	Transform2D test2;
	test2.pos = mousePos;
	test2.w = 1;
	test2.h = 1;

	//if (test.intersectPoint(mousePos))
	//	DrawRectangleLinesEx(test.getAABB(), 0.1f, GREEN);
	//else
	//	DrawRectangleLinesEx(test.getAABB(), 0.1f, BLUE);

	if (test.intersectTransform(test2))
	{
		DrawRectangleLinesEx(test.getAABB(), 0.1f, GREEN);
		DrawRectangleLinesEx(test2.getAABB(), 0.1f, GREEN);
	}
	else
	{
		DrawRectangleLinesEx(test.getAABB(), 0.1f, BLUE);
		DrawRectangleLinesEx(test2.getAABB(), 0.1f, RED);
	}



	// Anything drawn after this (e.g. HUD) uses raw screen coordinates, unaffected by the camera.
	EndMode2D();

#pragma region game_menu
#if PRODUCTION_BUILD == 0 || MENU
	if (showImGui)
	{
		ImGui::Begin("Menu");

		if (ImGui::BeginTabBar("MenuTabs"))
		{
			if (ImGui::BeginTabItem("Game Menu"))
			{

				ImGui::SeparatorText("Controls");
				ImGui::BulletText("Press 'TAB' to open/close the menu");
				ImGui::BulletText("Menu must be closed in order to place/break blocks");
				ImGui::BulletText("Press 'r' to reset world-gen settings to the default");
				ImGui::BulletText("'-' / '=': zoom out / in");
				ImGui::BulletText("'[' / ']': decrease / increase camera speed");

				//int cameraX = (int)gameData.camera.target.x;
				//if (cameraX >= 0 && cameraX < (int)savedTerrainNoise.size())
				//{
				//	float bn = savedTerrainNoise[cameraX];
				//	const char* branch =
				//		(bn > worldGen.plainThreshold - worldGen.terrainBlendZone &&
				//			bn < worldGen.plainThreshold + worldGen.terrainBlendZone) ? "BLEND" :
				//		(bn < worldGen.plainThreshold) ? "PLAINS" : "MOUNTAINS";
				//	ImGui::Text("Column %d  biomeNoise=%.5f  branch=%s", cameraX, bn, branch);
				//}

				ImGui::SeparatorText("Camera");
				ImGui::Text("Camera zoom:");  ImGui::SameLine(); ImGui::SliderFloat("##camZoom", &gameData.camera.zoom, MIN_CAM_ZOOM, MAX_CAM_ZOOM);
				HelpTooltip("How far in the camera is zoomed. Higher = closer view.");
				ImGui::Text("Camera speed:"); ImGui::SameLine(); ImGui::SliderFloat("##camSpeed", &gameData.cameraSpeed, MIN_CAM_SPEED, MAX_CAM_SPEED);
				HelpTooltip("How fast the camera pans with WASD.");

				ImGui::SeparatorText("World");
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.4f, 1.0f));
				ImGui::TextWrapped("Note: changing any world setting below regenerates the world, which may take a moment on large worlds.");
				ImGui::PopStyleColor();
				ImGui::Text("World width"); ImGui::SameLine();
				if (ImGui::SliderInt("##worldWidth", &worldWidth, 25, 10000))
					initGame(false, false);
				HelpTooltip("Width of the world in blocks. Changing this regenerates the world.");
				ImGui::Text("World height"); ImGui::SameLine();
				if (ImGui::SliderInt("##worldHeight", &worldHeight, 375, 1000))
					initGame(false, false);
				HelpTooltip("Height of the world in blocks. Changing this regenerates the world.");
				ImGui::Text("Seed:"); ImGui::SameLine();
				if (ImGui::InputInt("##seed", &seed))
					initGame(false, false);
				HelpTooltip("Seed for the random number generator. The same seed with the same settings always produces the same world.");

				ImGui::Text("Average world height: %d", worldGen.avgWorldHeight);
				HelpTooltip("Read-only: the mean surface height across every column, computed after generation.");
				


				ImGui::SeparatorText("Actions");
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

				ImGui::Checkbox("Show advanced settings", &showAdvancedSettings);

				if (showAdvancedSettings)
				{
					ImGui::SeparatorText("World Generation Settings");

					if (ImGui::CollapsingHeader("Mountain Settings"))
					{
						//ImGui::Text("Dirt mountain octaves:");   ImGui::SameLine(); ImGui::SliderInt("##dirtMtnOct", &worldGen.dirtMountainOctaves, 1, 20);
						ImGui::Text("Dirt mountain frequency:"); ImGui::SameLine();
						if (ImGui::SliderFloat("##dirtMtnFreq", &worldGen.dirtMountainFrequency, 0.00001f, 0.1f, "%.5f"))
							initGame(false, false);
						HelpTooltip("How rapidly the mountain dirt surface rises and falls. Higher = bumpier, more frequent hills; lower = broader, smoother mountains.");

						ImGui::Text("Min dirt mountain thickness:"); ImGui::SameLine();
						if (ImGui::SliderInt("##minDirtMtnThick", &worldGen.minDirtMountainThickness, 1, 50))
							initGame(false, false);
						HelpTooltip("Thinnest the dirt layer over stone can be on mountains (blocks).");
						ImGui::Text("Max dirt mountain thickness:"); ImGui::SameLine();
						if (ImGui::SliderInt("##maxDirtMtnThick", &worldGen.maxDirtMountainThickness, 1, 50))
							initGame(false, false);
						HelpTooltip("Thickest the dirt layer over stone can be on mountains (blocks).");

						//ImGui::Text("Stone mountain octaves:");  ImGui::SameLine(); ImGui::SliderInt("##stnMtnOct", &worldGen.stoneMountainOctaves, 1, 20);
						ImGui::Text("Stone mountain frequency:"); ImGui::SameLine();
						if (ImGui::SliderFloat("##stnMtnFreq", &worldGen.stoneMountainFrequency, 0.00001f, 0.1f, "%.5f"))
							initGame(false, false);
						HelpTooltip("How rapidly the mountain stone surface height varies. Higher = jaggier stone; lower = smoother.");
						ImGui::Text("Min stone mountain start:"); ImGui::SameLine();
						if (ImGui::SliderInt("##minStoneMtnStart", &worldGen.minStoneMountainStart, 330, 400))
							initGame(false, false);
						HelpTooltip("Highest the mountain stone surface can start, measured in blocks down from the top of the world. Smaller = taller peaks.");
						ImGui::Text("Max stone mountain start:"); ImGui::SameLine();
						if (ImGui::SliderInt("##maxStoneMtnStart", &worldGen.maxStoneMountainStart, 330, 400))
							initGame(false, false);
						HelpTooltip("Lowest the mountain stone surface can start, in blocks down from the top. Larger = deeper base.");
					}

					if (ImGui::CollapsingHeader("Plains Settings"))
					{
						//ImGui::Text("Dirt plain octaves:");   ImGui::SameLine(); ImGui::SliderInt("##dirtPlnOct", &worldGen.dirtPlainOctaves, 1, 20);
						ImGui::Text("Dirt plain frequency:"); ImGui::SameLine();
						if (ImGui::SliderFloat("##dirtPlnFreq", &worldGen.dirtPlainFrequency, 0.00001f, 0.1f, "%.5f"))
							initGame(false, false);
						HelpTooltip("How rapidly the plains dirt surface undulates. Higher = bumpier plains; lower = flatter.");

						ImGui::Text("Min dirt plain thickness:"); ImGui::SameLine();
						if (ImGui::SliderInt("##minDirtPlnThick", &worldGen.minDirtPlainThickness, 1, 50))
							initGame(false, false);
						HelpTooltip("Thinnest the dirt layer over stone can be on plains (blocks).");
						ImGui::Text("Max dirt plain thickness:"); ImGui::SameLine();
						if (ImGui::SliderInt("##maxDirtPlnThick", &worldGen.maxDirtPlainThickness, 1, 50))
							initGame(false, false);
						HelpTooltip("Thickest the dirt layer over stone can be on plains (blocks).");

						//ImGui::Text("Stone plain octaves:");  ImGui::SameLine(); ImGui::SliderInt("##stnPlnOct", &worldGen.stonePlainOctaves, 1, 20);
						ImGui::Text("Stone plain frequency:"); ImGui::SameLine();
						if (ImGui::SliderFloat("##stnPlnFreq", &worldGen.stonePlainFrequency, 0.00001f, 0.1f, "%.5f"))
							initGame(false, false);
						HelpTooltip("How rapidly the plains stone surface height varies.");

						ImGui::Text("Min stone plain start:"); ImGui::SameLine();
						if (ImGui::SliderInt("##minStonePlnStart", &worldGen.minStonePlainStart, 330, 400))
							initGame(false, false);
						HelpTooltip("Highest the plains stone surface can start, in blocks down from the top. Smaller = higher ground.");
						ImGui::Text("Max stone plain start:"); ImGui::SameLine();
						if (ImGui::SliderInt("##maxStonePlnStart", &worldGen.maxStonePlainStart, 330, 400))
							initGame(false, false);
						HelpTooltip("Lowest the plains stone surface can start, in blocks down from the top.");
					}

					if (ImGui::CollapsingHeader("Terrain and Biome Settings"))
					{
						//ImGui::Text("Terrain octaves:");  ImGui::SameLine(); ImGui::SliderInt("##terrOct", &worldGen.terrainOctaves, 1, 20);
						ImGui::Text("Terrain frequency:"); ImGui::SameLine();
						if (ImGui::SliderFloat("##terrFreq", &worldGen.terrainFrequency, 0.00001f, 0.01f, "%.5f"))
							initGame(false, false);
						HelpTooltip("How often terrain switches between plains and mountains across the world. Higher = alternates more often; lower = large continuous regions.");
						ImGui::Text("Terrain blend zone:"); ImGui::SameLine();
						if (ImGui::SliderFloat("##terrBlendZone", &worldGen.terrainBlendZone, 0.0f, 1.0f, "%.5f"))
							initGame(false, false);
						HelpTooltip("Width of the transition band between plains and mountains. Wider = longer, gentler slopes between them.");

						ImGui::Text("Min desert threshold:"); ImGui::SameLine();
						if (ImGui::SliderFloat("##minDesThresh", &worldGen.minDesertThreshold, 0.0f, 1.0f, "%.5f"))
							initGame(false, false);
						HelpTooltip("Lower edge of the biome-noise band that becomes desert.");
						ImGui::Text("Max desert threshold:"); ImGui::SameLine();
						if (ImGui::SliderFloat("##maxDesThresh", &worldGen.maxDesertThreshold, 0.0f, 1.0f, "%.5f"))
							initGame(false, false);
						HelpTooltip("Upper edge of the biome-noise band that becomes desert. The wider the min-max band, the more desert.");

						//ImGui::Text("Biome octaves:");  ImGui::SameLine(); ImGui::SliderInt("##bioOct", &worldGen.desertOctaves, 1, 20);
						ImGui::Text("Biome frequency:"); ImGui::SameLine();
						if (ImGui::SliderFloat("##desFreq", &worldGen.biomeFrequency, 0.00001f, 0.1f, "%.5f"))
							initGame(false, false);
						HelpTooltip("Size of biome regions. Higher = smaller, more frequent biomes; lower = large sprawling ones.");

						if (ImGui::Checkbox("Blend biomes", &worldGen.blendBiomes))
							initGame(false, false);
						HelpTooltip("Fade blocks across biome borders instead of a hard edge.");

						ImGui::Text("Biome blend radius:"); ImGui::SameLine();
						if (ImGui::SliderInt("##bioBlendRad", &worldGen.biomeBlendRadius, 0, 200))
							initGame(false, false);
						HelpTooltip("How many blocks wide the fade between neighboring biomes is (requires \"Blend biomes\").");
					}

					if (ImGui::CollapsingHeader("Cave and Tunnel Settings"))
					{
						if (ImGui::Checkbox("Generate caves", &worldGen.generateCaves))
						{
							initGame(false, false);
						}
						HelpTooltip("Toggle cave generation.");

						ImGui::Text("Wall start depth:"); ImGui::SameLine();
						if (ImGui::SliderInt("##wallStart", &worldGen.wallStartDepth, 0, 500))
							initGame(false, false);
						HelpTooltip("How many blocks below each column's surface the background wall begins. Lower = walls closer to the surface; the top blocks stay wall-free so surface caves open to the sky.");

						//ImGui::Text("Cave octaves:");  ImGui::SameLine();
						//if(ImGui::SliderInt("##caveOct", &worldGen.caveOctaves, 1, 10))
						//	initGame(false, false);
						//HelpTooltip("Roughness of cave edges. Higher = rougher, more jagged; lower = smoother, rounder.");
						ImGui::Text("Cave frequency:"); ImGui::SameLine();
						if (ImGui::SliderFloat("##caveFreq", &worldGen.caveFrequency, 0.00001f, 0.5f, "%.5f"))
							initGame(false, false);
						HelpTooltip("Scale of caves. Lower = large sprawling caverns; higher = small, dense, scattered pockets.");

						ImGui::Text("Min cave threshold:"); ImGui::SameLine();
						if (ImGui::SliderFloat("##minCaveThresh", &worldGen.minCaveThreshold, 0.0f, 1.0f, "%.5f"))
							initGame(false, false);
						HelpTooltip("Lower edge of the noise band that becomes a cave. The gap between min and max controls how open caves are.");
						ImGui::Text("Max cave threshold:"); ImGui::SameLine();
						if (ImGui::SliderFloat("##maxCaveThresh", &worldGen.maxCaveThreshold, 0.0f, 1.0f, "%.5f"))
							initGame(false, false);
						HelpTooltip("Upper edge of the noise band that becomes a cave. Wide min-max = open caverns; narrow = thin tunnels.");

						ImGui::Text("Cave open threshold:"); ImGui::SameLine();
						if (ImGui::SliderFloat("##caveOpenThresh", &worldGen.caveOpenThreshold, 0.0f, 1.0f, "%.5f"))
							initGame(false, false);
						HelpTooltip("How often caves break through to the surface. Higher = rarer surface openings; at 1.0 the surface stays sealed.");
						ImGui::Text("Max cave ceiling depth:"); ImGui::SameLine();
						if (ImGui::SliderFloat("##maxCaveCeilingDepth", &worldGen.maxCaveCeilingDepth, 0.0f, 300.0f, "%.1f"))
							initGame(false, false);
						HelpTooltip("In capped columns, the thickest layer of solid ground above the caves (blocks). Larger = caves start deeper below the surface.");

						if (ImGui::Checkbox("Generate tunnels", &worldGen.generateWorms))
						{
							initGame(false, false);
						}
						HelpTooltip("Toggle tunnel generation.");

						ImGui::Text("Current number of tunnels: %d", worldGen.curNumWorms);
						HelpTooltip("Read-only: how many tunnels were carved in the current world.");

						ImGui::Text("Min tunnel length:"); ImGui::SameLine();
						if (ImGui::SliderInt("##minTunlLen", &worldGen.minWormLength, 50, 500))
						{
							// min/maxWormWidth use getRandomInt(), which requires the first arg to be greater than or equal to the second arg
							if (worldGen.minWormLength > worldGen.maxWormLength)
								worldGen.maxWormLength = worldGen.minWormLength;
							initGame(false, false);
						}
						HelpTooltip("Shortest a tunnel can be, in steps.");
						ImGui::Text("Max tunnel length:"); ImGui::SameLine();
						if (ImGui::SliderInt("##maxTunlLen", &worldGen.maxWormLength, 50, 500))
						{
							if (worldGen.maxWormLength < worldGen.minWormLength)
								worldGen.minWormLength = worldGen.maxWormLength;
							initGame(false, false);
						}
						HelpTooltip("Longest a tunnel can be, in steps.");

						ImGui::Text("Min tunnel width:"); ImGui::SameLine();
						if (ImGui::SliderInt("##minTunlWidth", &worldGen.minWormWidth, 1, 20))
						{
							// min/maxWormWidth use getRandomInt(), which requires the first arg to be greater than or equal to the second arg
							if (worldGen.minWormWidth > worldGen.maxWormWidth)
								worldGen.maxWormWidth = worldGen.minWormWidth;
							initGame(false, false);
						}
						HelpTooltip("Narrowest a tunnel can be, in blocks.");
						ImGui::Text("Max tunnel width:"); ImGui::SameLine();
						if (ImGui::SliderInt("##maxTunlWidth", &worldGen.maxWormWidth, 1, 20))
						{
							if (worldGen.maxWormWidth < worldGen.minWormWidth)
								worldGen.minWormWidth = worldGen.maxWormWidth;
							initGame(false, false);
						}
						HelpTooltip("Widest a tunnel can be, in blocks.");

						ImGui::Text("Min tunnel turn angle:"); ImGui::SameLine();
						if (ImGui::SliderFloat("##minTunlAngle", &worldGen.minWormTurnAngle, -0.2f, 0.2f))
						{
							// min/maxWormTurnAngle use getRandomInt(), which requires the first arg to be greater than or equal to the second arg
							if (worldGen.minWormTurnAngle > worldGen.maxWormTurnAngle)
								worldGen.maxWormTurnAngle = worldGen.minWormTurnAngle;
							initGame(false, false);
						}
						HelpTooltip("Sharpest turn a tunnel can make toward one side each step. Wider range = twistier tunnels.");
						ImGui::Text("Max tunnel turn angle:"); ImGui::SameLine();
						if (ImGui::SliderFloat("##maxTunlAngle", &worldGen.maxWormTurnAngle, -0.2f, 0.2f))
						{
							if (worldGen.maxWormTurnAngle < worldGen.minWormTurnAngle)
								worldGen.minWormTurnAngle = worldGen.maxWormTurnAngle;
							initGame(false, false);
						}
						HelpTooltip("Sharpest turn a tunnel can make toward the other side each step. Narrow range = smoother sweeping curves.");
					}

					if (ImGui::CollapsingHeader("Special Material Settings"))
					{
						if (ImGui::Checkbox("Generate ore", &worldGen.generateOre))
							initGame(false, false);
						HelpTooltip("Toggle ore generation.");
						ImGui::Text("Ore threshold:"); ImGui::SameLine();
						if (ImGui::SliderInt("##oreThresh", &worldGen.oreThreshold, 0, 499))
							initGame(false, false);
						HelpTooltip("Highest point ores can appear, in blocks down from the top. Larger = ores restricted to deeper down.");
						ImGui::Text("Gold chance:"); ImGui::SameLine();
						if (ImGui::SliderFloat("##gldChance", &worldGen.goldChance, 0.0f, 1.0f, "%.5f"))
							initGame(false, false);
						HelpTooltip("Per-eligible-block chance to place gold.");
						ImGui::Text("Iron chance:"); ImGui::SameLine();
						if (ImGui::SliderFloat("##irnChance", &worldGen.ironChance, 1.0f, 0.0f, "%.5f"))
							initGame(false, false);
						HelpTooltip("Per-eligible-block chance to place iron.");
						ImGui::Text("Copper chance:"); ImGui::SameLine();
						if (ImGui::SliderFloat("##copChance", &worldGen.copperChance, 1.0f, 0.0f, "%.5f"))
							initGame(false, false);
						HelpTooltip("Per-eligible-block chance to place copper.");

						ImGui::Text("Ruby threshold:"); ImGui::SameLine();
						if (ImGui::SliderInt("##rubThresh", &worldGen.rubyThreshold, 0, 499))
							initGame(false, false);
						HelpTooltip("Depth (blocks down from the top) below which rubies and sapphires can appear.");
						ImGui::Text("Ruby chance:"); ImGui::SameLine();
						if (ImGui::SliderFloat("##rubChance", &worldGen.rubyChance, 0.0f, 1.0f, "%.5f"))
							initGame(false, false);
						HelpTooltip("Per-eligible-block chance to place a ruby or sapphire.");

						ImGui::Text("Clay threshold:"); ImGui::SameLine();
						if (ImGui::SliderInt("##clyThresh", &worldGen.clayThreshold, 0, 499))
							initGame(false, false);
						HelpTooltip("Depth (blocks down from the top) below which clay can replace dirt.");
						ImGui::Text("Clay chance:"); ImGui::SameLine();
						if (ImGui::SliderFloat("##clyChance", &worldGen.clayChance, 0.0f, 1.0f, "%.5f"))
							initGame(false, false);
						HelpTooltip("Chance for clay instead of dirt below the clay threshold.");
					}

					if (ImGui::CollapsingHeader("Tree Settings"))
					{
						if (ImGui::Checkbox("Generate trees", &worldGen.generateTrees))
							initGame(false, false);
						HelpTooltip("Toggle tree generation.");

						ImGui::Text("Tree spawn chance:"); ImGui::SameLine();
						if (ImGui::SliderFloat("##treeSpnChance", &worldGen.treeSpawnChance, 0.0f, 1.0f, "%.5f"))
							initGame(false, false);
						HelpTooltip("Per-surface-column chance to place a tree.");
					}
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Block Selection"))
			{
				ImGui::SeparatorText("Placing & Breaking Blocks");
				ImGui::TextWrapped("Close this menu (TAB) to edit the world. Select a block from the grid below, then:");
				ImGui::BulletText("Left click: break the block under the cursor");
				ImGui::BulletText("Right click: place the selected block");
				ImGui::BulletText("Middle click: pick the block under the cursor");
				ImGui::TextWrapped("While the menu is open, the mouse copies/pastes structures (see below) instead of placing or breaking blocks.");

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
						gameData.curBlock = i;
						gameData.nextBlockVariant = (i == Block::door) ? std::rand() % 2 : std::rand() % 4;
					}

					ImGui::PopID();

					// 10 buttons per row: SameLine keeps the next widget inline; skipping
					// it every 10th iteration drops to a new row.
					if (i % 10 != 0)
					{
						ImGui::SameLine();
					}
				}

				ImGui::Separator();

				ImGui::SeparatorText("Controls for Structures");
				ImGui::BulletText("'1': set start of selection area");
				ImGui::BulletText("'2': set end of selection area");
				ImGui::BulletText("Left CTRL + C: copy the selected area");
				ImGui::BulletText("Right click: paste the copied area");

				ImGui::SeparatorText("Saving & Loading Structures");
				ImGui::TextWrapped("To save a copied area, type a file name and press 'Save to file'. This creates a file you can load back any time.");
				ImGui::TextWrapped("To load a structure, type the file name and press 'Load from file', then right-click to paste it into the world.");

				ImGui::Text("File name:");
				ImGui::SameLine();
				ImGui::InputText("##structureFile", gameData.structureFile, sizeof(gameData.structureFile));

				if (ImGui::Button("Save to file"))
				{
					std::string path = RESOURCES_PATH "structures/";
					path += gameData.structureFile;
					path += ".bin";

					saveBlockDataToFile(
						curStructure.structureBlocks,
						curStructure.structureWallBlocks,
						curStructure.w,
						curStructure.h,
						path.c_str()
					);
				}

				if (ImGui::Button("Load from file"))
				{
					std::string path = RESOURCES_PATH "structures/";
					path += gameData.structureFile;
					path += ".bin";

					loadBlockDataFromFile(
						curStructure.structureBlocks,
						curStructure.structureWallBlocks,
						curStructure.w,
						curStructure.h,
						path.c_str()
					);
				}

				if (ImGui::Button("Copy World"))
				{
					gameData.selectionStart.x = 0;
					gameData.selectionStart.y = 0;
					gameData.selectionEnd.x = worldWidth - 1;
					gameData.selectionEnd.y = worldHeight - 1;

					curStructure.copyFromMap(
						gameData.gameMap,
						gameData.selectionStart,
						gameData.selectionEnd
					);
				}

				ImGui::Separator();

				if (ImGui::Checkbox("Preview structure", &gameData.previewStructure)) {}
				if (ImGui::Checkbox("Preview selection", &gameData.previewSelection)) {}

				ImGui::Text("Change the block shape to a 'm x n' grid:");
				if (ImGui::SliderInt("##blckShpX", &gameData.blockShape.x, 1, 100)) {}
				if (ImGui::SliderInt("##blckShpY", &gameData.blockShape.y, 1, 100)) {}

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
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