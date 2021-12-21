#include "Camera.h"


void Camera::processInput(double dTime)
{
	// KEYBOARD //
	double cameraSpeed = -5.0 * dTime;
	if (GetAsyncKeyState('W') & 0x8000) {
		cameraPos.x() += cameraSpeed * cameraFront.x();
		cameraPos.z() += cameraSpeed * cameraFront.z();
	}
	else if (GetAsyncKeyState('S') & 0x8000) {
		cameraPos.x() -= cameraSpeed * cameraFront.x();
		cameraPos.z() -= cameraSpeed * cameraFront.z();
	}
	if (GetAsyncKeyState('A') & 0x8000)
		cameraPos = cameraPos - normalize(cross(cameraFront, cameraUp)) * cameraSpeed;
	else if (GetAsyncKeyState('D') & 0x8000)
		cameraPos = cameraPos + normalize(cross(cameraFront, cameraUp)) * cameraSpeed;
	if (GetAsyncKeyState(' ') & 0x8000)
		cameraPos.y() -= cameraSpeed;
	else if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
		cameraPos.y() += cameraSpeed;

	// MOUSE //
	POINT pos;
	::GetCursorPos(&pos);
	::SetCursorPos(int(0.5 * m_width), int(0.5f * m_height));

	float xoffset = pos.x - m_lastX;
	float yoffset = pos.y - m_lastY; // reversed since y-coordinates go from bottom to top

	float sensitivity = 0.1f; // change this value to your liking
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	m_yaw += xoffset;
	m_pitch += yoffset;

	// make sure that when pitch is out of bounds, screen doesn't get flipped
	if (m_pitch > 89.0f)
		m_pitch = 89.0f;
	if (m_pitch < -89.0f)
		m_pitch = -89.0f;

	cameraFront = normalize(Vecd<3>{
		cos(M_PI* m_yaw / 180)* cos(M_PI* m_pitch / 180),
			sin(M_PI* m_pitch / 180),
			sin(M_PI* m_yaw / 180)* cos(M_PI* m_pitch / 180)
	});


	// Custom keys
	for (auto& key : customKeys)
	{
		if (!key.second)
		{
			if (GetKeyState(key.first) & 0x8000) {
				key.second = true;
				customKeysCallback(key.first, key.second);
			}
		}
		else
		{
			if (!(GetKeyState(key.first) & 0x8000)) {
				key.second = false;
				customKeysCallback(key.first, key.second);
			}
		}
	}
}

// Custom implementation of the LookAt function
Matd<4, 4> Camera::lookAt()
{
	// 1. Position = known
	// 2. Calculate cameraDirection
	Vecd<3> f = normalize(/* position +*/ cameraFront /* target */);
	// 3. Get positive right axis vector
	Vecd<3> s = normalize(cross(cameraUp/*worldUp*/, f));
	// 4. Calculate camera up vector
	Vecd<3> u = cross(f, s);

	// Create translation and rotation matrix
	// Return lookAt matrix as combination of translation and rotation matrix (first translation then rotation)
	return Matd<4, 4>{
		{ s.x(), s.y(), s.z(), -dot(s, cameraPos) },
		{ u.x(), u.y(), u.z(), -dot(u, cameraPos) },
		{ f.x(), f.y(), f.z(), -dot(f, cameraPos) },
		{ 1.0, 1.0, 1.0, 1.0 }
	};
}


Matd<4, 4> Camera::perspective(double aspect, double nearPlane, double farPlane)
{
	double tanHalfFovy(tan(M_PI * m_fov / 2 / 180.0f));

	Matd<4, 4> toRet{};
	toRet[0][0] = 1.0 / (aspect * tanHalfFovy);
	toRet[1][1] = 1.0 / (tanHalfFovy);
	toRet[2][2] = -(farPlane + nearPlane) / (farPlane - nearPlane);
	toRet[2][3] = -1.0;
	toRet[3][2] = -(2.0 * farPlane * nearPlane) / (farPlane - nearPlane);
	return toRet;
}


double Camera::timeSinceStart()
{
	return 1e-9 * (std::chrono::high_resolution_clock::now() - startTime).count();
}

void Camera::addTrackingKey(long keyId)
{
	customKeys.push_back({ keyId, false });
}

void Camera::removeTrackingKey(long keyId)
{
	auto it = std::find_if(customKeys.begin(), customKeys.end(),
		[keyId](const std::pair<long, bool>& el) { return el.first == keyId; });
	customKeys.erase(it);
}

void Camera::setCustomKeysCallback(void (*func)(long keyId, bool isPressed))
{
	customKeysCallback = func;
}