#pragma once
#include <Windows.h>

#include "Vecd.h"


class Texture
{
	HDC m_memContext;
	PUCHAR m_texPixels;
	size_t m_width, m_height, m_colorsCount;

public:
	Texture(const wchar_t* path)
	{
		// Setting up memory context
		m_memContext = CreateDC(L"DISPLAY", nullptr, nullptr, nullptr);
		if (!m_memContext)
			throw "Cant create memory context for texture";

		HBITMAP bitmap = (HBITMAP)LoadImageW(NULL, path, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

		if (!bitmap)
			throw "Cant load image";

		// So there is no stack corruption
		struct { BITMAPINFO info; RGBQUAD moreColors[255]; } bmpInfoLong{};
		BITMAPINFO &bmpInfo = bmpInfoLong.info;
		bmpInfo.bmiHeader.biSize = sizeof(bmpInfo.bmiHeader);

		// Get the BITMAPINFO structure from the bitmap
		if (!GetDIBits(m_memContext, bitmap, 0, 0, NULL, &bmpInfo, DIB_RGB_COLORS))
			throw "Cant read BMP data from image";

		m_width = bmpInfo.bmiHeader.biWidth;
		m_height = abs(bmpInfo.bmiHeader.biHeight - 1);
		m_colorsCount = bmpInfo.bmiHeader.biBitCount / 8;

		// create the bitmap buffer
		m_texPixels = new BYTE[bmpInfo.bmiHeader.biSizeImage]{};

		// get the actual bitmap buffer
		if (!GetDIBits(m_memContext, bitmap, 0, bmpInfo.bmiHeader.biHeight, m_texPixels, &bmpInfo, DIB_RGB_COLORS))
			throw "Cant get pixel data from image";
	}


	Vecd<4> getPixel(double x, double y)
	{
		Vecd<4> toRet{};
		x -= floor(x); y -= floor(y);
		if (x < 0) x += 1.0;
		if (y < 0) y += 1.0;
		size_t intX = size_t(x * m_width);
		size_t intY = size_t(y * m_height);

		PUCHAR colBegin = m_texPixels + intY * m_width * m_colorsCount + intX * m_colorsCount;
		for (size_t i(0); i < 4ULL; ++i)
			toRet[i] = colBegin[i] * 0.00390625;

		return toRet;
	}
};