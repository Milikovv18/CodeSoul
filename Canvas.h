#pragma once

#include <Windows.h>
#include <queue>
#include <sstream>
#include <memory>
#include <chrono>

#include "Figure.h"


class Canvas
{
private:
	const HWND m_windowHandler;
	const HDC m_context;
	const unsigned m_width;
	const unsigned m_height;
	const unsigned m_colorsCount;
	const double badW = 0.1;

	HDC m_memContext;
	HBITMAP m_frameBitmap;
	BITMAPINFO m_bitmapInfo;
	PUCHAR* m_ptrFramePixels;
	PUCHAR m_framePixels;

	// Aligning
	int m_horizAlign{ 0 };
	int m_vertAlign{ 0 };

	// Utils
	bool m_isDontCloseWindow{ false };
	bool m_isShowMSPF{ false };
	unsigned m_mspfCount{ 0 };
	std::chrono::high_resolution_clock::time_point m_timePoint;

	// Figures to draw on canvas
	std::queue<std::unique_ptr<IFigure>> figures;

public:
	struct Params
	{
		unsigned colorsCount = 3;
		bool dontCloseWindow = false;
		bool dontShowCursor = true;
		bool showMSPF = false;
	};

	enum Align
	{
		HCenter = 1,
		VCenter = 2,
		HVCenter = 4
	};

	// Construntors / destructors
	Canvas(HWND windowHandler, unsigned width, unsigned height, const Params& params);
	~Canvas();

	// Getters
	const size_t getPixelsSize() const;
	COLORREF getPixel(unsigned x, unsigned y) const;
	PUCHAR getArray() const;
	PUCHAR getArrayCopy() const;

	// Setters
	void setAlignment(int horizAlign, int vertAlign);
	void setAlignment(Align align, int horizAlign, int vertAlign);
	void setPixel(unsigned x, unsigned y, COLORREF rgb);
	void setArray(PUCHAR arr);
	void setArrayCopy(PUCHAR arr);

	// Renderers
	void fill(COLORREF rgb, float a = 1.0f);

	/// @brief Adds a new figure with relative coords (top left corner is [-1, -1])
	/// @param newFig Any figure with bounding box
	void addFigure(IFigure* newFig);
	void render();

	// Utils
	void showCursor(bool show) const;
	void dontCloseWindow(bool wait);
	double getScreenScaleFactor() const;
};

