// CodeSoul2.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>

#include "Canvas.h"
#include "Vecd.h"
#include "Matd.h"
#include "Figure.h"
#include "Texture.h"
#include "Camera.h"
#include "Physics.h"


double mix(double x, double y, double a)
{
	return x * (1 - a) + y * a;
}

uint8_t linear_to_srgb8(double col)
{
	return uint8_t(255 * col);
}


struct box2f
{
	Vecd<2> ul{};
	Vecd<2> lr{};
};

box2f join(const box2f& vp, const Vecd<2>& vec)
{
	box2f toRet{};
	toRet.ul.x() = vec.x() < vp.ul.x() ? vec.x() : vp.ul.x();
	toRet.ul.y() = vec.y() < vp.ul.y() ? vec.y() : vp.ul.y();
	toRet.lr.x() = vec.x() > vp.lr.x() ? vec.x() : vp.lr.x();
	toRet.lr.y() = vec.y() > vp.lr.y() ? vec.y() : vp.lr.y();
	return toRet;
}


struct Renderbuffer {
	int w, h, ys;
	void* data;
};

struct Vert {
	Vecd<4> position{};
	Vecd<4> texcoord{};
	Vecd<4> color{};
};

struct Varying {
	Vecd<4> texcoord{};
	Vecd<4> color{};
};

void vertex_shader(const Vert& in, Vecd<4>& gl_Position, Varying& out)
{
	out.texcoord = in.texcoord;
	out.color = in.color;
	gl_Position = in.position;
}

void fragment_shader(Vecd<4>& gl_FragCoord, const Varying& in, Vecd<4>& out)
{
	out = Vecd<4>{ 1, 0, 0, 0 }; // Blue color
	// Creates weird grid
	Vecd<2> wrapped = Vecd<2>(in.texcoord - floor(in.texcoord));
	bool brighter = (wrapped[0] < 0.5) != (wrapped[1] < 0.5);
	if (!brighter)
		(Vecd<3>&)out = 0.5f * (Vecd<3>&)out;
}

void store_color(Renderbuffer& buf, int x, int y, const Vecd<4>& c)
{
	// can do alpha composition here
	uint8_t* p = (uint8_t*)buf.data + buf.ys * ((int64_t)buf.h - y - 1) + (int64_t)4 * x;
	p[0] = linear_to_srgb8(c[0]);
	p[1] = linear_to_srgb8(c[1]);
	p[2] = linear_to_srgb8(c[2]);
	p[3] = (uint8_t)lrint(c[3] * 255);
}

void draw_triangle(Renderbuffer& color_attachment, const box2f& viewport, const Vert* verts)
{
	Varying perVertex[3]{};
	Vecd<4> gl_Position[3]{};

	box2f aabbf = { viewport.lr, viewport.ul };
	for (int i = 0; i < 3; ++i)
	{
		// invoke the vertex shader (writes given verts to gl_Position and fills perVertex with vert)
		vertex_shader(verts[i], gl_Position[i], perVertex[i]);

		// convert to device coordinates by perspective division (nothing changes) Point 1
		gl_Position[i][3] = 1 / gl_Position[i][3];
		gl_Position[i][0] *= gl_Position[i][3];
		gl_Position[i][1] *= gl_Position[i][3];
		gl_Position[i][2] *= gl_Position[i][3];

		// convert to window coordinates (from 0..1 to 0..512, changing only x and y) Point 2
		auto& pos2 = (Vecd<2>&)gl_Position[i];
		auto factor = 0.5f * (pos2 + Vecd<2>{1,1});
		pos2.x() = mix(viewport.ul.x(), viewport.lr.x(), factor.x());
		pos2.y() = mix(viewport.ul.y(), viewport.lr.y(), factor.y());
		aabbf = join(aabbf, (const Vecd<2>&)gl_Position[i]); // reducing triangle bounding box
	}

	// precompute the affine transform from fragment coordinates to barycentric coordinates (only 2D in window coords)
	// (counter-clockwise order)
	// Edge function (finds 1 divided by area of triangle with vertices gl_P[0], gl_P[1], gl_P[2])
	const double denomSquare = 1 / ((gl_Position[0][0] - gl_Position[2][0]) * (gl_Position[1][1] - gl_Position[0][1]) - (gl_Position[0][0] - gl_Position[1][0]) * (gl_Position[2][1] - gl_Position[0][1]));
	// Optimisation for interpolating all vertices: Y=Y0+λ1(Y1−Y0)+λ2(Y2−Y0)
	const Vecd<3> barycentric_d0 = denomSquare * Vecd<3>{gl_Position[1][1] - gl_Position[2][1], gl_Position[2][1] - gl_Position[0][1], gl_Position[0][1] - gl_Position[1][1]}; // Only Y
	// Optimisation for interpolating all vertices: X=X0+λ1(X1−X0)+λ2(X2−X0)
	const Vecd<3> barycentric_d1 = denomSquare * Vecd<3>{gl_Position[2][0] - gl_Position[1][0], gl_Position[0][0] - gl_Position[2][0], gl_Position[1][0] - gl_Position[0][0]}; // Only X
	// Optimisation for calculating sub-triangles areas (free member)
	const Vecd<3> barycentric_0 = denomSquare * Vecd<3>{
		gl_Position[1][0] * gl_Position[2][1] - gl_Position[2][0] * gl_Position[1][1],
		gl_Position[2][0] * gl_Position[0][1] - gl_Position[0][0] * gl_Position[2][1],
		gl_Position[0][0] * gl_Position[1][1] - gl_Position[1][0] * gl_Position[0][1]
	};

	// loop over all pixels in the rectangle bounding the triangle
	box2f aabb;
	aabb.ul.x() = lrint(aabbf.ul.x());
	aabb.ul.y() = lrint(aabbf.ul.y());
	aabb.lr.x() = lrint(aabbf.lr.x());
	aabb.lr.y() = lrint(aabbf.lr.y());
	for (int y = (int)aabb.ul.y(); y < aabb.lr.y(); ++y)
	{
		for (int x = (int)aabb.ul.x(); x < aabb.lr.x(); ++x)
		{
			Vecd<4> gl_FragCoord{}; // "P" point for barycentric coords
			gl_FragCoord[0] = x + 0.5;
			gl_FragCoord[1] = y + 0.5;

			// fragment barycentric coordinates in window coordinates (sub-triangles areas)
			const Vecd<3> barycentric = gl_FragCoord[0] * barycentric_d0 + gl_FragCoord[1] * barycentric_d1 + barycentric_0;

			// discard fragment outside the triangle. this doesn't handle edges correctly (because double almost never = 0).
			if (barycentric[0] < 0 || barycentric[1] < 0 || barycentric[2] < 0)
				continue;

			// interpolate inverse depth linearly (Z=Z0+λ1*Z1+λ2*Z2 and with W)
			gl_FragCoord[2] = dot(barycentric, Vecd<3>{gl_Position[0][2], gl_Position[1][2], gl_Position[2][2]}); // Z only
			gl_FragCoord[3] = dot(barycentric, Vecd<3>{gl_Position[0][3], gl_Position[1][3], gl_Position[2][3]}); // W only

			// clip fragments to the near/far planes (as if by GL_ZERO_TO_ONE)
			if (gl_FragCoord[2] < 0 || gl_FragCoord[2] > 1)
				continue;

			// convert to perspective correct (clip-space) barycentric (using only W)
			const Vecd<3> perspective = 1 / gl_FragCoord[3] * barycentric * Vecd<3>{gl_Position[0][3], gl_Position[1][3], gl_Position[2][3]};

			// interpolate the attributes using the perspective correct barycentric (using everything, changing perspective)
			Varying varying{};
			for (int i = 0; i < sizeof(Varying) / sizeof(double); ++i)
				((double*)&varying)[i] = dot(perspective, Vecd<3>{
					((const double*)&perVertex[0])[i],
					((const double*)&perVertex[1])[i],
					((const double*)&perVertex[2])[i]
			});

			// invoke the fragment shader and store the result
			Vecd<4> color{};
			fragment_shader(gl_FragCoord, varying, color);
			store_color(color_attachment, x, y, color); // Writes result "color" into array
		}
	}
}


const Vecd<4> lightPos{ -1.0, 0.0, 0.0, 0.0 };
const Vecd<3> lightCol{ 1.0, 1.0, 1.0 };
Camera cam(1024.0f, 512.0f, 90.0f);

Texture grassTex(LR"(grass.bmp)");
void floorFrag(const Vecd<4>& pos, const Vecd<4>& texture, Vecd<4>& col)
{
	Vecd<3> realPos = pos;
	realPos.z() = -pos.w();

	//col = Vecd<4>{ 1.0, 0.0, 0.0, 0.0 };
	col = grassTex.getPixel(texture.x(), texture.y());

	// Ambient
	Vecd<4> ambient = 0.3 * lightCol;

	// Result
	col = (ambient) * col;
}


Vecd<4> triagRotatedNormal;
Texture goldTex(LR"(gold.bmp)");
void triagFrag(const Vecd<4>& pos, const Vecd<4>& texture, Vecd<4>& col)
{
	//col = Vecd<4>{ 1.0, 0.0, 0.0, 0.0 };
	col = goldTex.getPixel(texture.x(), texture.y());

	Vecd<3> norm = normalize(triagRotatedNormal);
	Vecd<3> lightDir = normalize(lightPos - pos);

	// Ambient
	Vecd<4> ambient = 0.1 * lightCol;

	// Diffuse
	double diff = abs(max(dot(norm, lightDir), 0.0));
	Vecd<4> diffuse = 0.6 * diff * lightCol;

	// Specular
	Vecd<3> viewDir = normalize(cam.getPos() - pos);
	Vecd<3> reflectDir = reflect(-lightDir, norm);

	double spec = pow(max(dot(viewDir, reflectDir), 0.0), 64);
	Vecd<4> specular = 0.5 * spec * lightCol;

	// Result
	col = (ambient + diffuse) * col + specular;
}


bool changeForce(false);
void rightClickCallback(long keyId, bool isPressed)
{
	if (isPressed)
		changeForce = 1; // Apply
	else
		changeForce = 0; // Dont apply
}


int main()
{
	Canvas::Params cnvParams;
	cnvParams.colorsCount = 4;
	cnvParams.dontCloseWindow = true;
	cnvParams.showMSPF = true;

	Canvas cnv(GetConsoleWindow(), 1024, 512, cnvParams);
	cnv.setAlignment(Canvas::HVCenter, -512, -256);

	// Per vertex info
	Vecd<4> floorTexCoords[3][2]
	{
		// Textures
		{ {0.0, 10.0, 0.0, 1.0}, {}},
		{ {10.0, 0.0, 0.0, 1.0}, {}},
		{ {0.0, 0.0, 10.0, 1.0}, {}}
	};
	Vecd<4> thingTexCoords[3][2]
	{
		// Textures
		{ {0.0, 1.0, 0.0, 1.0}, {}},
		{ {1.0, 0.0, 0.0, 1.0}, {}},
		{ {0.0, 0.0, 1.0, 1.0}, {}}
	};

	const Vecd<3> thingVert[3]
	{
		{ -0.5, +1.5, +0.0 },
		{ -0.2, -1.0, +0.0 },
		{ +0.7, -0.5, +0.0 }
	};
	const Vecd<4> thingNormal{ 1.0, 0.0, 0.0, 0.0 };
	const Vecd<3> floorVert[3]
	{
		{ -7.0, -5.0, +0.0 },
		{ +7.0, -5.0, +7.0 },
		{ +0.0, -5.0, -5.0 }
	};
	const Vecd<4> floorNormal{ 0.0, 1.0, 0.0, 0.0 };

	const Vecd<4> normal = (thingVert[0] - thingVert[1]) * (thingVert[2] - thingVert[1]);
	const Vecd<3> offset{ 0.0, 0.0, 0.0 };

	// Simulation
	Matd<3, 3> IbodyInv = Matd<3, 3>
	{
		{ 0.58, 0.11, 0.00 },
		{ 0.11, 2.59, 0.00 },
		{ 0.00, 0.00, 0.47 }
	};

	const double Kws = 10.0;		// Hooke's spring constant
	const double Kwd = 10.0;		// Damping constant

	Simulator phySim(thingVert, IbodyInv, 1.0, 5.0);

	// View
	cam.setCustomKeysCallback(rightClickCallback);
	cam.addTrackingKey(VK_RBUTTON);

	// Perspective matrix
	float nearPlane(0.1f), farPlane(1.0f);
	Matd<4, 4> projMat(cam.perspective(1024.0 / 512.0, nearPlane, farPlane));

	double angle(0.0), deltaTime(0.0), lastTime(0.0);
	while (true)
	{
		Vecd<4> thingVert4[3], floorVert4[3];

		// Updates
		deltaTime = cam.timeSinceStart() - lastTime;
		lastTime = cam.timeSinceStart();
		cam.processInput(deltaTime);

		auto newThingVert = phySim.updatePhysics(deltaTime); // Translating and rotating
		for (int vId(0); vId < 3; ++vId)
		{
			thingVert4[vId] = newThingVert[vId];
			floorVert4[vId] = floorVert[vId];
			thingVert4[vId].w() = 1.0;
			floorVert4[vId].w() = 1.0;
		}

		if (changeForce == 1)
		{
			// Spring is connected to thingVert4[0]
			Vecd<3> anchor = cam.getPos() + (-5.0 * cam.getFront());

			if ((thingVert4[0] - anchor).sqLength() < 100.0)
			{
				Vecd<3> U = thingVert4[0] - phySim.getCenterPos();
				Vecd<3> VU = phySim.getLinearVelocity() +
					cross(phySim.getAngularVelocity(), U);

				Vecd<3> spring = -Kws * (thingVert4[0] - anchor);
				Vecd<3> dampingForce = spring + (-Kwd * (dot(VU, spring) / dot(spring, spring)) * spring);

				phySim.applyForce(dampingForce);
				phySim.applyTorque(cross(U, dampingForce));
			}
			else
			{
				if (anchor.y() <= 0)
					anchor.y() = 0.5;

				phySim.setCenterPos(anchor);
			}
		}

		triagRotatedNormal = cross(thingVert4[0] - thingVert4[1], thingVert4[2] - thingVert4[1]);

		auto lookMat = cam.lookAt();
		for (int i(0); i < 3; ++i)
		{
			// View
			thingVert4[i] = lookMat * thingVert4[i];
			floorVert4[i] = lookMat * floorVert4[i];

			// Projection
			thingVert4[i] = projMat * thingVert4[i];
			floorVert4[i] = projMat * floorVert4[i];
		}

		cnv.fill(RGB(0, 0, 0), 1.0f);

		auto floor = new Triangle<4>(floorVert4);
		floor->setFragmentShader(floorFrag);
		floor->setPerVertexInfo(floorTexCoords);
		cnv.addFigure(floor);

		auto triag = new Triangle<4>(thingVert4);
		triag->setFragmentShader(triagFrag);
		triag->setPerVertexInfo(thingTexCoords);
		cnv.addFigure(triag);

		cnv.render();
	}

	return 0;
}
