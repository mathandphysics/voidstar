#pragma once

#include <iostream>

class ScreenshotOverlay
{
public:
	ScreenshotOverlay();
	~ScreenshotOverlay();

	void NewScreenshotTaken(const std::string& fileName);
	bool ShouldShow();
	void OnImGuiRender();

private:
	double m_screenshotOverlayDuration = 4.0; // duration in seconds
	double m_screenshotTimeTaken = -m_screenshotOverlayDuration;
	std::string m_lastScreenshotFileName;
};