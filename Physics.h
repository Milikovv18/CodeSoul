#pragma once
#include <vector>

#include "Logger.h"

#include "Vecd.h"
#include "Matd.h"


struct Floor
{
	// Plane ax + by + cz + d = 0
	Vecd<3> normal{ 0.0, 1.0, 0.0 };
	double d = 0; // Always positive
};

struct Collision
{
	enum class Type
	{
		CLEAR, // No collision
		PENETRATING, // Inside a floor
		COLLIDING // Right before actual physical collision
	};

	Type state = Type::CLEAR;
	Vecd<3> normal{}; // Floor normal
	int vertexIndex = 0; // Colliding vertex
};

struct Coefficients
{
	const double restitutionCoef = 0.2;
	const double noKdl = 0.002;		// The no-damping linear damping factor
	const double noKda = 0.001;		// The no-damping angular damping factor
	const double timeEpsilon = 0.00001;
	const double colDepthEpsilon = 0.001;
	const double sFric = 0.5;		// Static friction
	const double dFric = 0.3;		// Dynamic friction
};

struct PhysicsConfig
{
	// States
	Vecd<3> vertices[3]; // Only triangles
	Vecd<3> pos{}; // Center of mass position
	Matd<3, 3> orientMat{ Matd<3, 3>::getIdentityMatrix() }; // Triangle orientation in body space

	Vecd<3> linearVel{};
	Vecd<3> angularMomentum{};

	// Derived (auxiliary)
	Matd<3, 3> Iinv;
	Vecd<3> angularVel;
	Matd<3, 3> rotMatDot;

	// Manually computed
	Vecd<3> sumForce;
	Vecd<3> sumTorque;
};


// Interface pattern
class Simulator
{
public:
	Simulator(const Vecd<3> vertices[3], const Matd<3, 3> inverseBodyInertiaTensor, double mass, double floorHeight) :
		IbodyInv(inverseBodyInertiaTensor), invMass(1.0 / mass)
	{
		for (int i(0); i < 3; ++i)
			bodyVertices[i] = vertices[i];

		if (floorHeight < 0.0)
			throw "Floor y can be only > 0.0";

		aFloor.d = floorHeight;
	}

	Vecd<3>* updatePhysics(double dTime);

	// Getters&setters
	void setFloorHeight(double floorHeight)
	{
		if (floorHeight < 0.0)
			throw "Floor y can be only > 0.0";
		aFloor.d = floorHeight;
	}

	void setCoefficients(const Coefficients& coefs)
	{
		memcpy(&this->coefs, &coefs, sizeof(Coefficients));
	}

	void setCenterPos(const Vecd<3>& newPos)
	{
		conf[SourceStateId].pos = newPos;
	}
	void setOrientation(Matd<3, 3> orientMat)
	{
		conf[SourceStateId].orientMat = orientMat;
	}

	void setLinearVelocity(const Vecd<3>& newVel)
	{
		conf[SourceStateId].linearVel = newVel;
	}
	void setAngularMomentum(const Vecd<3>& newMom)
	{
		conf[SourceStateId].angularMomentum = newMom;
	}

	void applyForce(const Vecd<3>& force)
	{
		applicableForces.push_back(force);
	}
	void applyTorque(const Vecd<3>& torque)
	{
		applicableTorques.push_back(torque);
	}


	double getFloorHeight()				{ return aFloor.d; }
	Coefficients& getCoefficients()		{ return coefs; }
	const Vecd<3>& getCenterPos()		{ return conf[SourceStateId].pos; }
	const Matd<3, 3>& getOrientation()	{ return conf[SourceStateId].orientMat; }
	const Vecd<3>& getLinearVelocity()	{ return conf[SourceStateId].linearVel; }
	const Vecd<3>& getAngularVelocity()	{ return conf[SourceStateId].angularVel; }
	const Vecd<3>& getAngularMomentum()	{ return conf[SourceStateId].angularMomentum; }

	double getInverseMass() { return invMass; }
	Matd<3, 3>& getInverseInertiaTensor() { return conf[SourceStateId].Iinv; }

private:
	PhysicsConfig conf[2]; // Source and target states
	int SourceStateId = 0;
	int TargetStateId = 1;

	std::vector<Vecd<3>> applicableForces;
	std::vector<Vecd<3>> applicableTorques;

	Collision coll;
	Floor aFloor;
	Coefficients coefs;

	/*const*/ Vecd<3> bodyVertices[3];

	const double invMass;
	const Matd<3, 3> IbodyInv;

	void computeForcesAndTorques(int stateId);
	void ode(double dTime);
	void updateVertices(int stateId);
	Collision::Type checkCollisions(int stateId);
	void resolveCollisions(int stateId);

	template<unsigned N, unsigned M> Matd<N, M> static star(const Vecd<N>& vec);
};
