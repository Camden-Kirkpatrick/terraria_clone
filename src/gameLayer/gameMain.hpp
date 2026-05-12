#pragma once
#define WIN_WIDTH 1920
#define WIN_HEIGHT 1080
#define FPS 240

bool initGame(bool resetNoise, bool resetCamera);

bool updateGame();

void closeGame();