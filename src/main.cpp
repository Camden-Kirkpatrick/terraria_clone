#include <iostream>
#include <raylib.h>
#include <imgui.h>
#include <rlImGui.h>

int main()
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(800, 600, "test");

	rlImGuiSetup(true);


	while (!WindowShouldClose())
	{
		BeginDrawing();
		ClearBackground(RAYWHITE);

		rlImGuiBegin();

		ImGui::Begin("test");

		ImGui::Text("hello");
		ImGui::Button("button");

		ImGui::End();


		DrawText("TEST", 100, 100, 20 , RED);

		DrawRectangle(175, 175, 100, 100, { 0, 255, 0, 255 });
		DrawRectangle(150, 150, 100, 100, { 255, 0, 0, 127 });

		rlImGuiEnd();
		EndDrawing();
	}

	rlImGuiShutdown();
	CloseWindow();

	return 0;
}