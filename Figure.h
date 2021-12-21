#pragma once

#include "Vecd.h"


struct CanvasData
{
	PUCHAR pixels;
	const unsigned width;
	const unsigned height;
	const unsigned colorsCount;
};

struct BoundingBox
{
	Vecd<2> upperLeft{};
	Vecd<2> lowerRight{};
};


__interface IFigure
{
	void draw(CanvasData& cd);
	void adaptBounds(BoundingBox& bbox, unsigned maxWidth, unsigned maxHeight, Vecd<2> newPoint);
	void storePixel(CanvasData& cd, size_t x, size_t y, Vecd<4>& color);
	Vecd<4>* getVertexArray();
};


template <unsigned N>
class Triangle : public IFigure
{
public:
	Triangle(Vecd<N> vertex[3]) : m_vertices{ vertex[0], vertex[1], vertex[2] }
	{
		fragmentShader = &defaultFragmentShader;
	}

	Triangle(const Triangle<N>& triag)
	{
		memcpy(m_vertices, triag.m_vertices, std::size(m_vertices) * sizeof(Vecd<N>));
		memcpy(m_perVertex, triag.m_perVertex, std::size(m_perVertex) * std::size(m_perVertex[0]) * sizeof(Vecd<N>));
		fragmentShader = triag.fragmentShader;
	}


	void draw(CanvasData& cd) override
	{
		BoundingBox bbox{ Vecd<2>{0.0, 0.0}, Vecd<2>{(double)cd.width, (double)cd.height} };

		// Iterate through every vertex
		for (int i(0); i < 3; ++i)
		{
			m_perVertex[0][i] = m_vertices[i];

			// Perspective division
			m_vertices[i][3] = 1 / m_vertices[i][3];
			m_vertices[i][0] *= m_vertices[i][3];
			m_vertices[i][1] *= m_vertices[i][3];
			m_vertices[i][2] *= m_vertices[i][3];

			// Convert to window space coords
			auto& pos2 = (Vecd<2>&)m_vertices[i];
			auto factor = 0.5f * (pos2 + Vecd<2>{1, 1});
			pos2.x() = mix(bbox.upperLeft.x(), bbox.lowerRight.x(), factor.x());
			pos2.y() = mix(bbox.upperLeft.y(), bbox.lowerRight.y(), factor.y());
			adaptBounds(bbox, cd.width, cd.height, (const Vecd<2>&)m_vertices[i]); // reducing triangle bounding box
		}

		// Precomps for barycentric coords
		// (1 / (entire triangle area))
		const double denomSquare = 1 / ((m_vertices[0][0] - m_vertices[2][0]) * (m_vertices[1][1] - m_vertices[0][1]) -
										(m_vertices[0][0] - m_vertices[1][0]) * (m_vertices[2][1] - m_vertices[0][1]));
		// Optimisation for P.x coord
		const Vecd<3> barycentric_Px = denomSquare *
			Vecd<3>{m_vertices[1][1] - m_vertices[2][1],
					m_vertices[2][1] - m_vertices[0][1],
					m_vertices[0][1] - m_vertices[1][1]};
		// Optimisation for P.y coord
		const Vecd<3> barycentric_Py = denomSquare *
			Vecd<3>{m_vertices[2][0] - m_vertices[1][0],
					m_vertices[0][0] - m_vertices[2][0],
					m_vertices[1][0] - m_vertices[0][0]};
		// Optimisation for free member
		const Vecd<3> barycentric_free = denomSquare *
			Vecd<3>{m_vertices[1][0] * m_vertices[2][1] - m_vertices[2][0] * m_vertices[1][1],
					m_vertices[2][0] * m_vertices[0][1] - m_vertices[0][0] * m_vertices[2][1],
					m_vertices[0][0] * m_vertices[1][1] - m_vertices[1][0] * m_vertices[0][1]};

		bbox.upperLeft.x() = lrint(bbox.upperLeft.x());
		bbox.upperLeft.y() = lrint(bbox.upperLeft.y());
		bbox.lowerRight.x() = lrint(bbox.lowerRight.x());
		bbox.lowerRight.y() = lrint(bbox.lowerRight.y());

		// Looping through every pixel in bounding box
		for (double y = (int)bbox.upperLeft.y(); y < bbox.lowerRight.y(); ++y)
		{
			for (double x = (int)bbox.upperLeft.x(); x < bbox.lowerRight.x(); ++x)
			{
				Vecd<4> fragCoord{ x, y };
				// Computing sub-triangle area divided by entire triangle area (barycentric coords)
				const Vecd<3> barycentric = fragCoord[0] * barycentric_Px +
					fragCoord[1] * barycentric_Py +
					barycentric_free;

				// Discard fragment outside the triangle
				if (barycentric[0] < 0 || barycentric[1] < 0 || barycentric[2] < 0)
					continue;

				// interpolate inverse depth linearly (Z=Z0+w1*Z1+w2*Z2 and with W, where w is barycentric)
				fragCoord[2] = dot(barycentric, Vecd<3>{m_vertices[0][2], m_vertices[1][2], m_vertices[2][2]}); // Z only
				fragCoord[3] = dot(barycentric, Vecd<3>{m_vertices[0][3], m_vertices[1][3], m_vertices[2][3]}); // W only

				// Clip fragments to the near/far planes (as if by GL_ZERO_TO_ONE)
				//if (fragCoord[2] < 0 || fragCoord[2] > 1)
				//	continue;

				// Perspective correct barycentric
				const Vecd<3> perspective = 1 / fragCoord[3] * barycentric *
					Vecd<3>{m_vertices[0][3], m_vertices[1][3], m_vertices[2][3]};

				// interpolate the attributes using the perspective correct barycentric
				Vecd<4> varying[3];
				for (int inf = 0; inf < 3; ++inf)
					for (int i = 0; i < sizeof(Vecd<4>) / sizeof(double); ++i)
						((double*)&varying[inf])[i] = dot(perspective, Vecd<3>{
						((const double*)&m_perVertex[inf][0])[i],
						((const double*)&m_perVertex[inf][1])[i],
						((const double*)&m_perVertex[inf][2])[i]
					});

				// Using fragment shader to do some colors
				Vecd<4> finalColor;
				fragmentShader(varying[0], varying[1], finalColor);
				storePixel(cd, (size_t)x, (size_t)y, finalColor);
			}
		}
	}

	static void defaultFragmentShader(const Vecd<N>& fragPosition, const Vecd<N>& texture, Vecd<4>& color)
	{
		color.r() = color.g() = color.b() = 1.0;
	}

	void setFragmentShader(void (*newFragmentShader)(const Vecd<N>& fragPosition, const Vecd<N>& texture, Vecd<4>& color))
	{
		fragmentShader = newFragmentShader;
	}

	/// @brief Some per vertex info, that should be interpolated
	/// @param info The actual info in form
	/// { { x11, y11, z11, w11... }, { x21, y21, z21, w21... }, ... },
	/// { { x12, y12, z12, w12... }, { x22, y22, z22, w22... }, ... },
	/// { { x13, y13, z13, w13... }, { x23, y23, z23, w23... }, ... }
	/// @param vexDensity How much info there is in every vertex
	void setPerVertexInfo(const Vecd<N> info[3][2])
	{
		for (int i(0); i < 2; ++i)
			for (int j(0); j < 3; ++j)
				m_perVertex[i + 1][j] = info[j][i];
	}


	Vecd<4>* getVertexArray()
	{
		return m_vertices;
	}

	~Triangle()
	{
		delete[] m_perVertex;
	}

private:
	Vecd<N> m_vertices[3]{};
	Vecd<N> m_perVertex[3][3]{};
	void (*fragmentShader)(const Vecd<N>& fragPosition, const Vecd<N>& texture, Vecd<4>& color) = nullptr;

	void adaptBounds(BoundingBox& bbox, unsigned maxWidth, unsigned maxHeight, Vecd<2> newPoint) override
	{
		bbox.upperLeft.x() = (newPoint.x() >= 0 && newPoint.x() < bbox.upperLeft.x()) ? newPoint.x() : bbox.upperLeft.x();
		bbox.upperLeft.y() = (newPoint.y() >= 0 && newPoint.y() < bbox.upperLeft.y()) ? newPoint.y() : bbox.upperLeft.y();
		bbox.lowerRight.x() = (newPoint.x() <= maxWidth && newPoint.x() > bbox.lowerRight.x()) ? newPoint.x() : bbox.lowerRight.x();
		bbox.lowerRight.y() = (newPoint.y() <= maxHeight && newPoint.y() > bbox.lowerRight.y()) ? newPoint.y() : bbox.lowerRight.y();
	}

	double mix(double x, double y, double prop) const
	{
		return x * (1 - prop) + y * prop;
	}

	void storePixel(CanvasData& cd, size_t x, size_t y, Vecd<4>& color) override
	{
		// Here we can rotate image ((cd.height - y - 1) * cd.width * cd.colorsCount)
		uint8_t* pixPos = cd.pixels + ((cd.height - y - 1) * cd.width * cd.colorsCount) + x * cd.colorsCount;
		pixPos[0] = uint8_t(min(255 * color[0], 255));
		pixPos[1] = uint8_t(min(255 * color[1], 255));
		pixPos[2] = uint8_t(min(255 * color[2], 255));
	}
};