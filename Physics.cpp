#include "Physics.h"


void Simulator::computeForcesAndTorques(int stateId)
{
	// Resetting
	conf[stateId].sumForce = {};
	conf[stateId].sumTorque = {};

	// Gravity
	conf[stateId].sumForce.y() += -9.8 / invMass;

	// Sum up all applicable forces and torques
	for (auto& force : applicableForces)
		conf[stateId].sumForce = conf[stateId].sumForce + force;

	for (auto& torque : applicableTorques)
		conf[stateId].sumTorque = conf[stateId].sumTorque + torque;

	applicableForces.clear();
	applicableTorques.clear();

	// Little dumping
	conf[stateId].sumForce = conf[stateId].sumForce + (-coefs.noKdl * conf[stateId].linearVel);
	conf[stateId].sumTorque = conf[stateId].sumTorque + (-coefs.noKda * conf[stateId].angularVel);
}


template<unsigned N, unsigned M> Matd<N, M> Simulator::star(const Vecd<N>& vec)
{
	return Matd<3, 3>
	{
		{ 0.0, -vec.z(), +vec.y() },
		{ +vec.z(), 0.0, -vec.x() },
		{ -vec.y(), +vec.x(), 0.0 },
	};
}


void Simulator::ode(double dTime)
{
	auto& source = conf[SourceStateId];
	auto& target = conf[TargetStateId];

	target.pos = source.pos + (source.linearVel * dTime);

	target.rotMatDot = star<3, 3>(source.angularVel) * source.orientMat;
	target.orientMat = source.orientMat + (target.rotMatDot * dTime);
	target.orientMat.orthonormalize3D();

	target.linearVel = source.linearVel + (dTime * invMass) * source.sumForce;
	target.angularMomentum = source.angularMomentum + (source.sumTorque * dTime);

	// Auxiliary //
	target.Iinv = target.orientMat * IbodyInv * transpose(target.orientMat);
	target.angularVel = target.Iinv * target.angularMomentum;
}


void Simulator::updateVertices(int stateId)
{
	for (int vId(0); vId < 3; ++vId)
		conf[stateId].vertices[vId] = conf[stateId].pos + (conf[stateId].orientMat * bodyVertices[vId]);
}


Collision::Type Simulator::checkCollisions(int stateId)
{
	coll.state = Collision::Type::CLEAR;

	for (int vId(0); (vId < 3) && (coll.state != Collision::Type::PENETRATING); ++vId)
	{
		Vecd<3> vertPos = conf[stateId].vertices[vId];
		Vecd<3> U = vertPos - conf[stateId].pos;

		Vecd<3> fullVel = conf[stateId].linearVel + cross(conf[stateId].angularVel, U);

		double wallDist = dot(vertPos, aFloor.normal) + aFloor.d;
		if (wallDist < -coefs.colDepthEpsilon)
		{
			coll.state = Collision::Type::PENETRATING;
		}
		else if (wallDist < coefs.colDepthEpsilon)
		{
			double relVel = dot(aFloor.normal, fullVel);
			if (relVel < 0.0)
			{
				coll.state = Collision::Type::COLLIDING;
				coll.normal = aFloor.normal;
				coll.vertexIndex = vId;
			}
		}
	}

	return coll.state;
}


void Simulator::resolveCollisions(int stateId)
{
	auto& state = conf[stateId];

	Vecd<3> R = state.vertices[coll.vertexIndex] - state.pos;
	Vecd<3> fullVel = state.linearVel + cross(state.angularVel, R);

	double reactImpulseNumerator = -(1.0 + coefs.restitutionCoef) * dot(fullVel, coll.normal);
	double reactImpulseDenominator = invMass +
		dot(cross(state.Iinv * cross(R, coll.normal), R), coll.normal);
	double reactImpulseMag = (reactImpulseNumerator / reactImpulseDenominator);
	Vecd<3> reactImpulse = reactImpulseMag * coll.normal;

	// Frictions //
	Vecd<3> tangent = normalize(fullVel);
	double staticFrictionMag = coefs.sFric * reactImpulseMag;
	double dynamicFrictionMag = coefs.dFric * reactImpulseMag;
	Vecd<3> frictionImpulse;
	if (dot(fullVel, tangent) == 0 && dot(fullVel / invMass, tangent) <= staticFrictionMag)
		frictionImpulse = -(dot(fullVel / invMass, tangent)) * tangent;
	else
		frictionImpulse = -dynamicFrictionMag * tangent;

	reactImpulse = reactImpulse + frictionImpulse;

	// Applying impulse
	state.linearVel = state.linearVel + invMass * reactImpulse;
	state.angularMomentum = state.angularMomentum + cross(R, reactImpulse);

	// Auxilliary
	state.angularVel = state.Iinv * state.angularMomentum;
}


Vecd<3>* Simulator::updatePhysics(const double dTime)
{
	double currentTime = 0.0;
	double targetTime = dTime;

	while (currentTime < dTime)
	{
		computeForcesAndTorques(SourceStateId);

		ode(targetTime - currentTime);

		updateVertices(TargetStateId);

		checkCollisions(TargetStateId);

		if (coll.state == Collision::Type::PENETRATING)
		{
			targetTime = (currentTime + targetTime) * 0.5;

			// If it crashes here, impulse (resolveCollisions()) might not be able to correct collision with current restitutionCoef 
			if (fabs(targetTime - currentTime) <= coefs.timeEpsilon)
				throw "Interpenetrated at the start of a frame";

			continue;
		}

		if (coll.state == Collision::Type::COLLIDING)
		{
			int counter(0);
			do
			{
				resolveCollisions(TargetStateId);
				counter++;
			} while ((checkCollisions(TargetStateId) == Collision::Type::COLLIDING) && (counter < 100));

			if (counter >= 100)
				throw "Too much";
		}

		// No collision
		currentTime = targetTime;
		targetTime = dTime;

		SourceStateId = SourceStateId ? 0 : 1;
		TargetStateId = TargetStateId ? 0 : 1;
	}

	return conf[SourceStateId].vertices;
}