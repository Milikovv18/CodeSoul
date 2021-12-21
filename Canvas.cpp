#include "Canvas.h"


Canvas::Canvas(HWND windowHandler, unsigned width, unsigned height, const Params& params)
	: m_windowHandler(windowHandler), m_width(width), m_height(height),
	m_colorsCount(params.colorsCount), m_context(::GetDC(m_windowHandler))
{
	if (!m_context)
		throw("Cant get device context");

	m_memContext = CreateCompatibleDC(m_context);
	if (!m_memContext)
		throw("Cant get memory device context");

	ZeroMemory(&m_bitmapInfo, sizeof(BITMAPINFO));
	m_bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_bitmapInfo.bmiHeader.biWidth = width;
	m_bitmapInfo.bmiHeader.biHeight = -(long)height;		// Negative, so the origin is upper-left corner
	m_bitmapInfo.bmiHeader.biPlanes = 1;					// Must be 1
	m_bitmapInfo.bmiHeader.biBitCount = 8 * m_colorsCount;	// m_colorsCount bytes for every pixel
	m_bitmapInfo.bmiHeader.biCompression = BI_RGB;			// Uncompressed
	m_bitmapInfo.bmiHeader.biSizeImage = 0;					// Can be 0 if uncompressed (autodetected)
	m_bitmapInfo.bmiHeader.biXPelsPerMeter = 0;				// Nevermind
	m_bitmapInfo.bmiHeader.biYPelsPerMeter = 0;				// Nevermind
	m_bitmapInfo.bmiHeader.biClrUsed = m_colorsCount;		// 3 different colors used
	m_bitmapInfo.bmiHeader.biClrImportant = m_colorsCount;	// Like previous one
	m_bitmapInfo.bmiColors; // NULL

	m_ptrFramePixels = &m_framePixels;

	// Initializing actual bitmap and getting pixels array
	m_frameBitmap = ::CreateDIBSection(m_memContext, &m_bitmapInfo, DIB_RGB_COLORS, (void**)m_ptrFramePixels, 0, 0);
	if (!m_frameBitmap)
		throw("Cant generate pixels for bitmap");

	// Selecting new canvas
	HGDIOBJ oldObj = ::SelectObject(m_memContext, m_frameBitmap);
	if (!oldObj || oldObj == HGDI_ERROR)
		throw("Cant select new bitmap");

	// Fill entire canvas with white
	memset(m_framePixels, 255, getPixelsSize());

	// Additional settings
	dontCloseWindow(params.dontCloseWindow);
	showCursor(!params.dontShowCursor);
	m_isShowMSPF = params.showMSPF;
}

Canvas::~Canvas()
{
	::DeleteDC(m_memContext);
	::DeleteObject(m_frameBitmap);

	if (m_isDontCloseWindow) Sleep(INFINITE);
}



void Canvas::setAlignment(int horizAlign, int vertAlign)
{
	RECT windowRect;
	::GetClientRect(m_windowHandler, &windowRect);

	// VERY IMPORTANT // if you have screen scaling
	double factor = getScreenScaleFactor();
	windowRect.right = long(windowRect.right * factor);
	windowRect.bottom = long(windowRect.bottom * factor);

	// Horizontal
	m_horizAlign = horizAlign;
	if (horizAlign < 0)
		m_horizAlign += windowRect.right - m_width;
	
	// Vertical
	m_vertAlign = vertAlign;
	if (vertAlign < 0)
		m_vertAlign += windowRect.bottom - m_height;
}

void Canvas::setAlignment(Align align, int horizAlign, int vertAlign)
{
	RECT windowRect;
	::GetClientRect(m_windowHandler, &windowRect);

	// VERY IMPORTANT // if you have screen scaling
	double factor = getScreenScaleFactor();
	windowRect.right = long(windowRect.right * factor);
	windowRect.bottom = long(windowRect.bottom * factor);

	if (align & (Align::HVCenter | Align::HCenter))
		horizAlign += windowRect.right / 2;
	if (align & (Align::HVCenter | Align::VCenter))
		vertAlign += windowRect.bottom / 2;

	// Horizontal
	m_horizAlign = horizAlign;
	if (horizAlign < 0)
		m_horizAlign += windowRect.right - m_width;

	// Vertical
	m_vertAlign = vertAlign;
	if (vertAlign < 0)
		m_vertAlign += windowRect.bottom - m_height;
}

void Canvas::setPixel(unsigned x, unsigned y, COLORREF rgb)
{
	if (x < 0 || y < 0 || x >= m_width || y >= m_height)
		return;

	//if (a < 0) a = 0;
	//if (a > 1) a = 1;

	int index = y * m_width * m_colorsCount + x * m_colorsCount;
	rgb = _byteswap_ulong(rgb) >> 8;
	memcpy(&m_framePixels[index], &rgb, m_colorsCount);
}

void Canvas::setArray(PUCHAR arr)
{
	if (!::SetDIBits(m_memContext, m_frameBitmap, 0, m_height, arr, &m_bitmapInfo, DIB_RGB_COLORS))
		throw("Cant set the exact array");
}

void Canvas::setArrayCopy(PUCHAR arr)
{
	memcpy(m_framePixels, arr, getPixelsSize());
}



const size_t Canvas::getPixelsSize() const
{
	return (size_t)m_width * m_height * m_colorsCount;
}

COLORREF Canvas::getPixel(unsigned x, unsigned y) const
{
	if (x < 0 || y < 0 || x > m_width || y > m_height)
		return RGB(0, 0, 0);

	int idx = y * m_width * m_colorsCount + x * m_colorsCount;
	return _byteswap_ulong((COLORREF&)m_framePixels[idx] << 8);
}

PUCHAR Canvas::getArray() const
{
	return m_framePixels;
}

PUCHAR Canvas::getArrayCopy() const
{
	PUCHAR newArr = new UCHAR[getPixelsSize()];
	memcpy(newArr, m_framePixels, getPixelsSize());
	return newArr;
}



void Canvas::fill(COLORREF rgb, float a)
{
	for (unsigned y(0); y < m_height; ++y)
		for (unsigned x(0); x < m_width; ++x)
		{
			COLORREF prev = getPixel(x, y);
			char R = GetRValue(prev) * (1.0f - a) + GetRValue(rgb) * a;
			char G = GetGValue(prev) * (1.0f - a) + GetGValue(rgb) * a;
			char B = GetBValue(prev) * (1.0f - a) + GetBValue(rgb) * a;
			setPixel(x, y, RGB(R, G, B));
		}
}

void Canvas::addFigure(IFigure* newFig)
{
	// Checking where w < 0
	Vecd<4>* vertices = newFig->getVertexArray();
	unsigned badVertIds[3]{}, goodVertIds[3]{}, badOffset(0), goodOffset(0);
	unsigned badCount(0);
	for (int i(0); i < 3; ++i)
	{
		if (vertices[i].w() <= badW)
		{
			badVertIds[badOffset++] = i;
			badCount++;
		}
		else {
			goodVertIds[goodOffset++] = i;
		}
	}

	// Discarding bad vertices ONLY FOR TRIANGLES
	if (badCount > 0)
	{
		if (badCount == 3)
		{
			return;
		}
		else if (badCount == 2)
		{
			for (unsigned i(0); i < badCount; ++i)
			{
				double wDiff = vertices[goodVertIds[0]].w() - vertices[badVertIds[i]].w();
				double factorX = (vertices[goodVertIds[0]].x() - vertices[badVertIds[i]].x()) / wDiff;
				double factorY = (vertices[goodVertIds[0]].y() - vertices[badVertIds[i]].y()) / wDiff;
				double factorZ = (vertices[goodVertIds[0]].z() - vertices[badVertIds[i]].z()) / wDiff;
				vertices[badVertIds[i]] = {
					vertices[badVertIds[i]].x() - (vertices[badVertIds[i]].w() - badW) * factorX,
					vertices[badVertIds[i]].y() - (vertices[badVertIds[i]].w() - badW) * factorY,
					vertices[badVertIds[i]].z() - (vertices[badVertIds[i]].w() - badW) * factorZ, badW
				};
			}
		}
		else if (badCount == 1)
		{
			Vecd<4> newVert[2];
			for (unsigned i(0); i < 3 - badCount; ++i)
			{
				double wDiff = vertices[goodVertIds[i]].w() - vertices[badVertIds[0]].w();
				double factorX = (vertices[goodVertIds[i]].x() - vertices[badVertIds[0]].x()) / wDiff;
				double factorY = (vertices[goodVertIds[i]].y() - vertices[badVertIds[0]].y()) / wDiff;
				double factorZ = (vertices[goodVertIds[i]].z() - vertices[badVertIds[0]].z()) / wDiff;
				newVert[i] = {
					vertices[badVertIds[0]].x() - (vertices[badVertIds[0]].w() - badW) * factorX,
					vertices[badVertIds[0]].y() - (vertices[badVertIds[0]].w() - badW) * factorY,
					vertices[badVertIds[0]].z() - (vertices[badVertIds[0]].w() - badW) * factorZ, badW
				};
			}

			vertices[badVertIds[0]] = newVert[0];
			auto newTriag = new Triangle<4>(*dynamic_cast<Triangle<4>*>(newFig));
			figures.push(std::unique_ptr<IFigure>(newTriag));

			vertices[goodVertIds[0]] = newVert[1];
		}
	}

	figures.push(std::unique_ptr<IFigure>(newFig));
}

void Canvas::render()
{
	using namespace std::chrono;
	CanvasData cd{ m_framePixels, m_width, m_height, m_colorsCount };

	if (m_isShowMSPF) {
		m_timePoint = high_resolution_clock::now();
	}

	while (!figures.empty())
	{
		figures.front()->draw(cd);
		figures.pop();
	}

	// Stretching, never needed
	/*if (!::StretchBlt(m_context, m_horizAlign, m_vertAlign, m_width, m_height,
		m_memContext, 0, 0, m_width, m_height, SRCCOPY))
		throw("Cant stretch and draw pixels");*/

	if (!::BitBlt(m_context, m_horizAlign, m_vertAlign, m_width, m_height,
		m_memContext, 0, 0, SRCCOPY))
		throw("Cant stretch and draw pixels");

	if (m_isShowMSPF)
	{
		std::wstringstream name;
		name << duration_cast<milliseconds>(high_resolution_clock::now() - m_timePoint).count() << L"ms";
		SetWindowText(m_windowHandler, name.str().c_str());
	}
}



void Canvas::showCursor(bool show) const
{
	// Hide cursor
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);

	// Hiding cursor
	CONSOLE_CURSOR_INFO sci;
	GetConsoleCursorInfo(handle, &sci);
	sci.bVisible = show;
	SetConsoleCursorInfo(handle, &sci);
}

void Canvas::dontCloseWindow(bool wait)
{
	m_isDontCloseWindow = wait;
}

double Canvas::getScreenScaleFactor() const
{
	// Getting false screen dimensions
	auto scaled = GetSystemMetrics(SM_CXSCREEN);

	// Getting true screen dimensions
	HKEY hKey;
	LONG lRes = RegOpenKeyEx(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_READ, &hKey);
	if (lRes == ERROR_FILE_NOT_FOUND)
		throw("Cant find registry directory");

	WCHAR szBuffer[2];
	DWORD dwBufferSize = sizeof(szBuffer);
	ULONG nError = RegQueryValueEx(hKey, L"MaxMonitorDimension", 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
	if (nError != ERROR_SUCCESS)
		throw("Cant read value");
	RegCloseKey(hKey);

	return (double)szBuffer[0] / scaled;
}