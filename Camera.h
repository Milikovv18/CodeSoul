#pragma once

#include <Windows.h>
#include <corecrt_math_defines.h>
#include <chrono>
#include <vector>

#include "Vecd.h"
#include "Matd.h"


class Camera
{
public:
	Camera(float screenWidth, float screenHeight, float FoV = 45.0f)
		: m_width(screenWidth), m_height(screenHeight), m_lastX(0.5f * m_width), m_lastY(0.5f * m_height), m_fov(FoV)
	{
		::SetCursorPos((int)m_lastX, (int)m_lastY);
		startTime = std::chrono::high_resolution_clock::now();
	}

	void processInput(double dTime);

	// Custom implementation of the LookAt and perspective functions
	Matd<4, 4> lookAt();
	Matd<4, 4> perspective(double ratio, double nearPlane, double farPlane);

	Vecd<3> getPos() { return cameraPos; }
	Vecd<3> getFront() { return cameraFront; }

	double timeSinceStart();
	void addTrackingKey(long keyId);
	void removeTrackingKey(long keyId);
	void setCustomKeysCallback(void (*func)(long keyId, bool isPressed));

private:
	Vecd<3> cameraPos{ 0.0f, 0.0f, 3.0f };
	Vecd<3> cameraFront{ 0.0f, 0.0f, -1.0f };
	Vecd<3> cameraUp{ 0.0f, 1.0f, 0.0f };
	std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> startTime;

	// Custom keys like mouse button { keyNo, keyState }
	void (*customKeysCallback)(long keyId, bool isPressed) = nullptr;
	std::vector<std::pair<long, bool>> customKeys{};

	float m_width;
	float m_height;
	float m_yaw = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
	float m_pitch = 0.0f;
	float m_lastX;
	float m_lastY;
	float m_fov;
};