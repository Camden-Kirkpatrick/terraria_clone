#pragma once
#define WIN_WIDTH 800
#define WIN_HEIGHT 600
#define FPS 240

bool initGame(bool resetNoise, bool resetCamera);

bool updateGame();

void closeGame();