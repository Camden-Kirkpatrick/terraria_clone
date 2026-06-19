#pragma once
#define WIN_WIDTH 1920
#define WIN_HEIGHT 1080
#define FPS 240
#define MENU 1 // If enabled in the production build, the user can change settings in game

bool initGame(bool resetWorldGen, bool resetCamera);

bool updateGame();

void closeGame();