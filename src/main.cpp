#include <iostream>
#include <raylib.h>
#include <imgui.h>

int main()
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(800, 600, "test");

	while (!WindowShouldClose())
	{
		BeginDrawing();
		ClearBackground(RAYWHITE);

		//DrawText("TEST", 100, 100, 20 , RED);

		DrawRectangle(75, 75, 100, 100, { 0, 255, 0, 255 });
		DrawRectangle(50, 50, 100, 100, { 255, 0, 0, 127 });
	
		EndDrawing();
	}

	CloseWindow();

	return 0;
}