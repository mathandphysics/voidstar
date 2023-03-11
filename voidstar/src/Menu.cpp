#include "Menu.h"

#include "imgui.h"
#include "Application.h"


Menu::Menu()
{
}

Menu::~Menu()
{
}

void Menu::OnUpdate()
{
}

void Menu::OnRender()
{
}

void Menu::OnImGuiRender()
{
	if (m_showSceneMenu)
	{
		Menu::ShowSceneMenu();
	}

	if (m_showFileMenu)
	{
		Menu::ShowFileMenu();
	}

	if (m_showCameraControls)
	{
		Application::Get().GetCamera().OnImGuiRender();
	}
}

void Menu::ShowSceneMenu()
{
	ImGuiWindowFlags imgui_window_flags = ImGuiWindowFlags_NoCollapse;// | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
	imgui_window_flags |= ImGuiWindowFlags_HorizontalScrollbar;
	ImGui::Begin("Scene Menu", NULL, imgui_window_flags);

	if (m_showFPS)
	{
		ImGui::Text("%.1f FPS Average", ImGui::GetIO().Framerate);
	}

	Application& app = Application::Get();
	scene::SceneManager& manager = app.GetSceneManager();
	std::vector<std::pair<std::string, std::function<scene::Scene* ()>>>& scenes = manager.GetSceneList();
	for (auto& scene : scenes)
	{
		if (ImGui::Button(scene.first.c_str()))
		{
			manager.DeleteCurrentScene();
			manager.SetCurrentScene(scene.second);
		}
	}

	if (ImGui::Button("Quit"))
	{
		Application::Get().OnWindowClose();
	}

	ImGui::End();
}

void Menu::ShowSceneDropdown()
{
	Application& app = Application::Get();
	scene::SceneManager& manager = app.GetSceneManager();
	std::vector<std::pair<std::string, std::function<scene::Scene* ()>>>& scenes = manager.GetSceneList();
	if (ImGui::BeginMenu("Scenes"))
	{
		for (auto& scene : scenes)
		{
			if (ImGui::MenuItem(scene.first.c_str()))
			{
				manager.DeleteCurrentScene();
				manager.SetCurrentScene(scene.second);
			}
		}
		ImGui::EndMenu();
	}
}

void Menu::ShowFileMenu()
{
	Application& app = Application::Get();
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Quit"))
			{
				app.OnWindowClose();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Options"))
		{
			app.GetCamera().CameraGUI();
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("View"))
		{
			//ImGui::Checkbox("Show Scene List Window", &m_showSceneMenu);
			ImGui::Checkbox("Show Camera Controls", &m_showCameraControls);
			//ImGui::Checkbox("Show FPS in Scene Window", &m_showFPS);
			ImGui::EndMenu();
		}

		//Menu::ShowSceneDropdown();
		Menu::ShowHelp();

		ImGui::EndMainMenuBar();
	}
}

void Menu::ShowHelp()
{
	bool openAboutModal = false;
	bool openControlsModal = false;
	if (ImGui::BeginMenu("Help"))
	{
		if (ImGui::MenuItem("Controls"))
		{
			openControlsModal = true;
		}
		if (ImGui::MenuItem("About"))
		{
			openAboutModal = true;
		}

		ImGui::EndMenu();
	}

	if (openControlsModal)
	{
		ImGui::OpenPopup("Controls");
	}

	if (openAboutModal)
	{
		ImGui::OpenPopup("About");
	}

	Menu::ShowControlsModal(openControlsModal);
	Menu::ShowAboutModal(openAboutModal);
}

void Menu::ShowAboutModal(bool& showModal)
{
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar;
	if (ImGui::BeginPopupModal("About", NULL, flags))
	{
		char longestLine[] = "An OpenGL graphics renderer written in C++ with:";
		ImGui::Text(longestLine);
		ImGui::Text("\nglfw for windowing\nglad for OpenGL function loading\nglm for math\nDear ImGui for the GUI\nstb_image for reading and writing images\n");

		ImVec2 vMin = ImGui::GetWindowContentRegionMin();
		ImVec2 vMax = ImGui::GetWindowContentRegionMax();
		ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + ImGui::CalcTextSize(longestLine).x);
		ImGui::TextWrapped("\nThis is a small, personal project that I plan to use as a basis for future math and physics based graphics programs.");
		ImGui::PopTextWrapPos();

		ImGui::Text("\nEnjoy!");

		ImGui::Separator();

		if (ImGui::Button("Cool story, bro"))
		{
			showModal = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void Menu::ShowControlsModal(bool& showModal)
{
	std::vector<std::vector<std::string>> controls = {
	{"Esc", "Show/Hide GUI"},
	{"W", "Forward"},
	{"S", "Backward"},
	{"A", "Left"},
	{"D", "Right"},
	{"Spacebar", "Up"},
	{"Left Shift", "Down"},
	{"Mouse Wheel", "Change Movement Speed"},
	{"Right Click", "Take Screenshot"}
	};

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove;// | ImGuiWindowFlags_NoTitleBar;
	if (ImGui::BeginPopupModal("Controls", NULL, flags))
	{
		ImGuiTableFlags table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame;
		if (ImGui::BeginTable("Controls Table", 2, table_flags))
		{
			ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableHeadersRow();
			ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGui::GetStyle().Colors[ImGuiCol_Button]));
			for (int row = 0; row < controls.size(); row++)
			{
				ImGui::TableNextRow();
				for (int column = 0; column < 2; column++)
				{
					ImGui::TableSetColumnIndex(column);
					ImGui::Text("%s", controls[row][column].c_str());
				}
			}
			ImGui::EndTable();
		}

		ImGui::Separator();

		if (ImGui::Button("Got it!"))
		{
			showModal = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}