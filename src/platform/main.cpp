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
	InitWindow(win_width, win_height, "test");
	SetTargetFPS(FPS);

#pragma region imgui
	//rlImGuiSetup(true);

	//// Allow docking and increase font size
	//ImGuiIO& io = ImGui::GetIO();
	//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	//io.FontGlobalScale = 2;

	//// Change the style/theme
	////ImGui::StyleColorsClassic();
	//ImGui::SetupImGuiCatppuccinMochaStyle();
#pragma endregion

	if (!initGame())
	{
		return 1;
	}

	while (!WindowShouldClose())
	{
		BeginDrawing();
		ClearBackground(BLACK);

	#pragma region imgui
		//rlImGuiBegin();

		//// Allow ImGui windows to dock to the game window
		//ImGui::PushStyleColor(ImGuiCol_WindowBg, {});
		//ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, {});
		//ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
		//ImGui::PopStyleColor(2);
		//
		//// Basic ImGui demo
		//ImGui::Begin("test");

		//ImGui::Text("hello");

		//if (ImGui::Button("button"))
		//	std::cout << "button pressed\n";
		//ImGui::SameLine();
		//if (ImGui::Button("button2"))
		//	std::cout << "button2 pressed\n";

		//ImGui::End();

		//// See all the fetures that ImGui has
		////ImGui::ShowDemoWindow();

		//ImGui::Begin("second window");

		//ImGui::Text("hello");
		//ImGui::Separator();
		//ImGui::NewLine();
		//
		//// Update a variable using a slider
		//static float a = 0;
		//ImGui::SliderFloat("slider", &a, 0, 1);

		//static char buffer[128] = "";

		//// Enter text and display it
		//ImGuiInputTextFlags flags =
		//	ImGuiInputTextFlags_CharsNoBlank |        // no spaces
		//	ImGuiInputTextFlags_EnterReturnsTrue |    // return true on Enter
		//	ImGuiInputTextFlags_AutoSelectAll;        // select all text when focused

		//if (ImGui::InputText("Player Name", buffer, sizeof(buffer), flags))
		//{
		//	std::cout << "Confirmed name: " << buffer << "\n";
		//}

		//// Color selector
		//static float color[4]{};
		//ImGui::ColorEdit4("color", color);
		//
	
		//ImGui::End();
	#pragma endregion


		if (!updateGame())
		{
			CloseWindow();
		}

	#pragma region imgui
		//rlImGuiEnd();
	#pragma endregion

		EndDrawing();
	}

#pragma region imgui
	//rlImGuiShutdown();
#pragma endregion

	CloseWindow();

	closeGame();

	return 0;
}