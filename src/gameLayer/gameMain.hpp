#pragma once
#define WIN_WIDTH 1920
#define WIN_HEIGHT 1080
#define FPS 240
#define MENU 1 // Enable to allow the user to change various settings

bool initGame(bool resetWorldGen, bool resetCamera);

bool updateGame();

void closeGame();