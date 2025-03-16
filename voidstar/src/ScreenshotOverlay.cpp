#include "ScreenshotOverlay.h"

#include "imgui.h"
#include "GLFW/glfw3.h"

ScreenshotOverlay::ScreenshotOverlay()
{
}
ScreenshotOverlay::~ScreenshotOverlay()
{
}

void ScreenshotOverlay::NewScreenshotTaken(const std::string& fileName)
{
	m_Screenshots.push_back(ScreenshotInfo{ fileName, glfwGetTime() });
}

void ScreenshotOverlay::OnRender()
{
	double currentTime = glfwGetTime();
	auto erased = std::erase_if(m_Screenshots,
		[&](const ScreenshotInfo info) {return currentTime - info.timeTaken > m_screenshotOverlayDuration; });
	if (!m_Screenshots.empty())
	{
		OnImGuiRender();
	}
}
void ScreenshotOverlay::OnImGuiRender()
{
	// Get screen position for the overlay window.
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
	ImVec2 work_size = viewport->WorkSize;
	const float PAD = 10.0f;
	ImVec2 window_pos = { work_pos.x + PAD, work_pos.y + PAD };
	ImVec2 window_pos_pivot = { 0.0f, 0.0f };

	ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
	ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize;
	window_flags |= ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
	window_flags |= ImGuiWindowFlags_NoMove;
	if (ImGui::Begin("ScreenshotInfoWindow", NULL, window_flags))
	{
		ImGui::Text(m_Screenshots.size() > 1 ? "Screenshots saved!" : "Screenshot saved!");
		for (auto& info : m_Screenshots)
		{
			{
				ImGui::Text("%s", info.filename.c_str());
			}
		}
		ImGui::End();
	}
}