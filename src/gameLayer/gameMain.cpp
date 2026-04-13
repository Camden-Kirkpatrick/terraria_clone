#include "gameMain.hpp"
#include <raylib.h>
#include <fstream>
#include <iostream>

struct GameData
{
	float posX = 0;
	float posY = 0;
	int playerWidth = 50;
	int playerHeight = 50;
	Color c{255, 0, 200, 255};
}gameData;

bool initGame()
{
	return true;
}

bool updateGame()
{
	//DrawText("TEST", 100, 100, 20, RED);

	//DrawRectangle(150, 150, 100, 100, { 255, 0, 0, 127 });
	//DrawRectangle(175, 175, 100, 100, { 0, 255, 0, 255 });


	// Player movement
	if (IsKeyDown(KEY_A)) { gameData.posX -= 1; }
	if (IsKeyDown(KEY_D)) { gameData.posX += 1; }
	if (IsKeyDown(KEY_W)) { gameData.posY -= 1; }
	if (IsKeyDown(KEY_S)) { gameData.posY += 1; }


	// Prevent the player from going out of bounds
	if (gameData.posX < 0) gameData.posX = 0;
	if (gameData.posX + gameData.playerWidth > win_width)
		gameData.posX = win_width - gameData.playerWidth;
	 
	if (gameData.posY < 0) gameData.posY = 0;
	if (gameData.posY + gameData.playerHeight > win_height)
		gameData.posY = win_height - gameData.playerHeight;


	// Wrap the player to the left/right/top/bottom of the screen
 //   if (gameData.posX + gameData.playerWidth > win_width)
	//	gameData.posX = 0;
	//if (gameData.posX < 0)
	//	gameData.posX = win_width - gameData.playerWidth;

	//if (gameData.posY + gameData.playerHeight > win_height)
	//	gameData.posY = 0;
	//if (gameData.posY < 0)
	//	gameData.posY = win_height - gameData.playerHeight;
	

	// Allow the player to wrap smoothy (pixel by pixel), instead of all at once
	//for (int i = gameData.posY; i < gameData.posY + gameData.playerHeight; i++)
	//{
	//	for (int j = gameData.posX; j < gameData.posX + gameData.playerWidth; j++)
	//	{
	//		int wrappedX = ((j % win_width) + win_width) % win_width;
	//		int wrappedY = ((i % win_height) + win_height) % win_height;
	//		DrawPixel(wrappedX, wrappedY, gameData.c);
	//	}
	//}

	DrawRectangle(gameData.posX, gameData.posY, gameData.playerWidth, gameData.playerHeight, gameData.c);

	return true;
}

void closeGame()
{
	std::ofstream f(RESOURCES_PATH "f.txt");
	f << "CLOSED\n";
	f.close();
}