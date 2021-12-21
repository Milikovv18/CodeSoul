#pragma once

#include <memory>
#include "Vecd.h"


template <unsigned N, unsigned M>
class Matd
{
	double value[N][M]{};

public:
	Matd() {};
	Matd(std::initializer_list<std::initializer_list<double>> il)
	{
		for (int i(0); i < il.size(); ++i) {
			memcpy(value[i], (il.begin() + i)->begin(), (il.begin() + i)->size() * sizeof(double));
		}
	}

	Matd(const Matd<N, M>& mat)
	{
		memcpy(value, mat.value, N * M * sizeof(double));
	}
	Matd<N, M> operator=(const Matd<N, M>& mat)
	{
		memcpy(value, mat.value, N * M * sizeof(double));
		return *this;
	}

	static Matd<N, N> getIdentityMatrix()
	{
		Matd<N, N> toRet{};
		for (int i(0); i < N; ++i)
			toRet[i][i] = 1.0;
		return toRet;
	}


	double* operator[](unsigned i) { return value[i]; }
	const double* operator[](unsigned i) const { return value[i]; }
	void setRow(unsigned num, std::initializer_list<double> il)
	{
		memcpy(value[num], il.begin(), il.size() * sizeof(double));
		if (M > il.size())
			memset(value[num] + il.size(), 0, (N - il.size()) * sizeof(double));
	}
	void setCol(unsigned num, std::initializer_list<double> il)
	{
		for (int i(0); i < N; ++i)
			value[num][i] = *(il.begin() + i);
		if (N > il.size())
			memset(value[num] + il.size(), 0, (N - il.size()) * sizeof(double));
	}

	// Matrix and vector multiplication
	Vecd<N> operator*(const Vecd<N>& vec)
	{
		Vecd<N> toRet{};
		for (int row(0); row < N; ++row)
		{
			double tempRes(0.0);
			for (int col(0); col < M; ++col)
				tempRes += value[row][col] * vec[col];
			toRet[row] = tempRes;
		}
		return toRet;
	}

	// Matrix and matrix multiplication
	Matd<N, M> operator+(const Matd<N, M>& mat)
	{
		Matd<N, M> toRet{};
		for (int row(0); row < N; ++row)
			for (int col(0); col < M; ++col)
				toRet[row][col] = value[row][col] + mat[row][col];
		return toRet;
	}

	// Matrix and matrix multiplication
	Matd<N, M> operator*(const Matd<M, N>& mat)
	{
		Matd<N, M> toRet{};
		for (int row(0); row < N; ++row)
		{
			for (int col(0); col < M; ++col)
			{
				double tempRes(0.0);
				for (int k(0); k < N; ++k)
					tempRes += value[row][k] * mat[k][col];
				toRet[row][col] = tempRes;
			}
		}
		return toRet;
	}

	// Matrix and scalar multiplication
	Matd<N, M> operator*(double num)
	{
		Matd<N, M> toRet{};
		for (int row(0); row < N; ++row)
			for (int col(0); col < M; ++col)
				toRet[row][col] = value[row][col] * num;
		return toRet;
	}

	// Matrix division by scalar
	Matd<N, M> operator/(double num)
	{
		Matd<N, M> toRet{};
		double invNum = 1 / num;
		for (int row(0); row < N; ++row)
			for (int col(0); col < M; ++col)
				toRet[row][col] = value[row][col] * invNum;
		return toRet;
	}


	void orthonormalize3D()
	{
		Vecd<3> X{ value[0][0], value[1][0], value[2][0] };
		Vecd<3> Y{ value[0][1], value[1][1], value[2][1] };
		Vecd<3> Z;

		X = normalize(X);
		Z = normalize(cross(X, Y));
		Y = normalize(cross(Z, X));

		value[0][0] = X[0]; value[0][1] = Y[0]; value[0][2] = Z[0];
		value[1][0] = X[1]; value[1][1] = Y[1]; value[1][2] = Z[1];
		value[2][0] = X[2]; value[2][1] = Y[2]; value[2][2] = Z[2];
	}
};


//// OUTER CLASS FUNCTIONS ////


template <unsigned N, unsigned M>
Matd<M, N> transpose(const Matd<N, M>& mat)
{
	Matd<M, N> toRet;
	for (int i(0); i < M; ++i)
		for (int j(0); j < N; ++j)
			toRet[i][j] = mat[j][i];
	return toRet;
}