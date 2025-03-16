#pragma once

#include <iostream>
#include <vector>

struct ScreenshotInfo
{
	std::string filename;
	double timeTaken;
};

class ScreenshotOverlay
{
public:
	ScreenshotOverlay();
	~ScreenshotOverlay();

	void NewScreenshotTaken(const std::string& fileName);
	void OnRender();
	void OnImGuiRender();

private:
	double m_screenshotOverlayDuration = 4.0; // duration in seconds
	std::vector<ScreenshotInfo> m_Screenshots;
};