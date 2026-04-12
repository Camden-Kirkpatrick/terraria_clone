#include <iostream>
#include <raylib.h>
#include <imgui.h>
#include <rlImGui.h>

int main()
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(800, 600, "test");

#pragma region imgui
	rlImGuiSetup(true);

	// Allow docking and increase font size
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.FontGlobalScale = 2;
#pragma endregion

	while (!WindowShouldClose())
	{
		BeginDrawing();
		ClearBackground(RAYWHITE);

	#pragma region imgui
		rlImGuiBegin();

		// Allow ImGui windows to dock to the game window
		ImGui::PushStyleColor(ImGuiCol_WindowBg, {});
		ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, {});
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
		ImGui::PopStyleColor(2);
		
		// Basic ImGui demo
		ImGui::Begin("test");

		ImGui::Text("hello");

		if (ImGui::Button("button"))
			std::cout << "button pressed\n";
		ImGui::SameLine();
		if (ImGui::Button("button2"))
			std::cout << "button2 pressed\n";

		ImGui::End();

		// See all the fetures that ImGui has
		//ImGui::ShowDemoWindow();

		ImGui::Begin("second window");

		ImGui::Text("hello");
		ImGui::Separator();
		ImGui::NewLine();
		
		// Update a variable using a slider
		static float a = 0;
		ImGui::SliderFloat("slider", &a, 0, 1);
	
		ImGui::End();
	#pragma endregion


		DrawText("TEST", 100, 100, 20 , RED);

		DrawRectangle(175, 175, 100, 100, { 0, 255, 0, 255 });
		DrawRectangle(150, 150, 100, 100, { 255, 0, 0, 127 });

	#pragma region imgui
		rlImGuiEnd();
	#pragma endregion

		EndDrawing();
	}

#pragma region imgui
	rlImGuiShutdown();
#pragma endregion

	CloseWindow();

	return 0;
}