#pragma once
#include <memory>
#include <initializer_list>


template <unsigned N>
class Vecd
{
	union
	{
		double value[N]{};
		struct { double xCoord, yCoord, zCoord, wCoord; };
		struct { double rColor, gColor, bColor, aColor; };
	};

public:
	Vecd() {};
	Vecd(std::initializer_list<double> il)
	{
		memcpy(value, il.begin(), il.size() * sizeof(double));
		if (N > il.size())
			memset(value + il.size(), 0, (N - il.size()) * sizeof(double));
	}

	template <unsigned O>
	Vecd(const Vecd<O>& other) // Cant be const and reference
	{
		unsigned len = (O < N ? O : N) * sizeof(double);
		memcpy(value, &other, len);
		if (N > len)
			memset(value + O, 0, (N - len) * sizeof(double));
	}
	template <unsigned O>
	Vecd<N> operator=(const Vecd<O>& other)
	{
		unsigned len = (O < N ? O : N) * sizeof(double);
		memcpy(value, &other, len);
		if (N > len)
			memset(value + O, 0, (N - len) * sizeof(double));
		return *this;
	}


	double& x() { return xCoord; }
	double x() const { return xCoord; }
	double& y() { return yCoord; }
	double y() const { return yCoord; }
	double& z() { return zCoord; }
	double z() const { return zCoord; }
	double& w() { return wCoord; }
	double w() const { return wCoord; }
	double& r() { return rColor; }
	double r() const { return rColor; }
	double& g() { return gColor; }
	double g() const { return gColor; }
	double& b() { return bColor; }
	double b() const { return bColor; }
	double& a() { return aColor; }
	double a() const { return aColor; }

	double sqLength() const
	{
		double squaredLen(0);
		for (int i(0); i < N; ++i)
			squaredLen += value[i] * value[i];
		return squaredLen;
	}

	double& operator[](unsigned i) { return value[i]; }
	double operator[](unsigned i) const { return value[i]; }

	Vecd<N> operator-() const
	{
		Vecd<N> toRet{};
		for (int i(0); i < N; ++i)
			toRet[i] = -(*this)[i];
		return toRet;
	}

	Vecd<N> operator+(const Vecd<N>& other) const
	{
		Vecd<N> toRet{};
		for (int i(0); i < N; ++i)
			toRet[i] = (*this)[i] + other[i];
		return toRet;
	}
	Vecd<N> operator-(const Vecd<N>& other) const
	{
		return (*this) + (-1) * other;
	}
	Vecd<N> operator*(const Vecd<N>& other) const
	{
		Vecd<N> toRet{};
		for (int i(0); i < N; ++i)
			toRet[i] = (*this)[i] * other[i];
		return toRet;
	}

	Vecd<N> operator+(double val) const
	{
		Vecd<N> toRet{};
		for (int i(0); i < N; ++i)
			toRet[i] = (*this)[i] + val;
		return toRet;
	}
	Vecd<N> operator-(double val) const
	{
		return (*this) + -val;
	}
	Vecd<N> operator*(double val) const
	{
		Vecd<N> toRet{};
		for (int i(0); i < N; ++i)
			toRet[i] = (*this)[i] * val;
		return toRet;
	}
	Vecd<N> operator/(double val) const
	{
		double invVal = 1.0 / val;
		return (*this) * invVal;
	}
};


//// OUTER CLASS FUNCTIONS ////


template <unsigned N>
Vecd<N> operator*(double val, const Vecd<N>& vec)
{
	return vec * val;
}

template<unsigned N>
Vecd<N> floor(const Vecd<N>& vec)
{
	Vecd<N> toRet{};
	for (int i(0); i < N; ++i)
		toRet[i] = floor(vec[i]);
	return toRet;
}

template<unsigned N>
double dot(const Vecd<N>& first, const Vecd<N>& second)
{
	Vecd<N> meta{ first * second };
	double sum(0);
	for (int i(0); i < N; ++i)
		sum += meta[i];
	return sum;
}

// 3D ONLY
template<unsigned N>
Vecd<N> cross(const Vecd<N>& first, const Vecd<N>& second)
{
	if (N < 3)
		throw "Not enough dimensions";

	return Vecd<3>{
		first[1] * second[2] - second[1] * first[2],
		first[2] * second[0] - second[2] * first[0],
		first[0] * second[1] - second[0] * first[1]
	};
}

template <unsigned N>
Vecd<N> normalize(const Vecd<N>& vec)
{
	float squaredLen = (float)vec.sqLength();

	// Inverse square root
	const float threehalfs = 1.5F;
	float invRoot = squaredLen; // ONLY FLOAT
	long i = *(long*)&invRoot;
	i = 0x5f3759df - (i >> 1);
	invRoot = *(float*)&i;
	invRoot *= threehalfs - ((squaredLen * 0.5F) * invRoot * invRoot);

	Vecd<N> normalized{};
	for (int i(0); i < N; ++i)
		normalized[i] = vec[i] * invRoot;

	return normalized;
}

template <unsigned N>
Vecd<N> reflect(const Vecd<N>& toReflect, const Vecd<N>& norm)
{
	return toReflect - 2.0 * dot(toReflect, norm) * norm;
}