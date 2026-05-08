#include <iostream>
#include <raylib.h>
#include <imgui.h>
#include <rlImGui.h>
#include "imguiThemes.hpp"
#include "gameMain.hpp"



int main()
{
#if PRODUCTION_BUILD == 1
	SetTraceLogLevel(LOG_NONE); // Don't open the console for the production build
#endif

	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(WIN_WIDTH, WIN_HEIGHT, "game");
	SetTargetFPS(FPS);

#pragma region imgui
	rlImGuiSetup(true);

	// Allow docking and increase font size
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.FontGlobalScale = 2;

	// Change the style/theme
	//ImGui::StyleColorsClassic();
	ImGui::SetupImGuiCatppuccinMochaStyle();
#pragma endregion

	if (!initGame(true, true))
	{
		return 1;
	}

	while (!WindowShouldClose())
	{
		BeginDrawing();
		ClearBackground(BLACK);

	#pragma region imgui
		rlImGuiBegin();

		// Allow ImGui windows to dock to the game window
		ImGui::PushStyleColor(ImGuiCol_WindowBg, {});
		ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, {});
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
		ImGui::PopStyleColor(2);
	#pragma endregion


		if (!updateGame())
		{
			CloseWindow();
		}

	#pragma region imgui
		rlImGuiEnd();
	#pragma endregion

		EndDrawing();
	}

#pragma region imgui
	rlImGuiShutdown();
#pragma endregion

	CloseWindow();

	closeGame();

	return 0;
}