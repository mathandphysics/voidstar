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
	m_lastScreenshotFileName = fileName;
	m_screenshotTimeTaken = glfwGetTime();
}

bool ScreenshotOverlay::ShouldShow()
{
	if (glfwGetTime() - m_screenshotTimeTaken < m_screenshotOverlayDuration)
	{
		return true;
	}
	else
	{
		return false;
	}
}
void ScreenshotOverlay::OnImGuiRender()
{
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize;
	window_flags |= ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
	const float PAD = 10.0f;
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
	ImVec2 work_size = viewport->WorkSize;
	ImVec2 window_pos, window_pos_pivot;
	window_pos.x = (work_pos.x + PAD);
	window_pos.y = (work_pos.y + PAD);
	window_pos_pivot.x = 0.0f;
	window_pos_pivot.y = 0.0f;
	ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
	window_flags |= ImGuiWindowFlags_NoMove;
	ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
	if (ImGui::Begin("Example: Simple overlay", NULL, window_flags))
	{
		ImGui::Text("Screenshot saved:\n%s", m_lastScreenshotFileName.c_str());
	}
	ImGui::End();
}