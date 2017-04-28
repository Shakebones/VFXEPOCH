/*******************************************************************************
    VFXEPOCH - Physically based simulation VFX

    Copyright (c) 2016 Snow Tsui <trevor.miscellaneous@gmail.com>

    All rights reserved. Use of this source code is governed by
    the MIT license as written in the LICENSE file.
*******************************************************************************/

// This utility is usually used for testing grids within several digits.
// Higher dimensions could violate the display format.

#include "Helpers.h"

const int Nx = 5;
const int Ny = 8;
const float source = 1.0f;
const float spacex = 1.0f / (Nx + 1);
const float spcaey = 1.0f / (Ny + 1);

using namespace Helpers;

int main(int argc, char** argv)
{
	std::cout << '\n' << "VFXEpoch Lib - Ubuntu v14.04 Unit Test" << '\n' << '\n';

	VFXEpoch::Grid2DfScalarField gridf(Nx, Ny);
	VFXEpoch::Grid2DVector2DfField gridv(Nx, Ny);
	Helpers::randomInitScalarField(gridf, -1.0, 1.0);
	Helpers::randomInitVectorField(gridv, -1.0, 1.0);

	cout << "2D Scalar field access test:" << endl;
	displayScalarField(gridf);
	cout << endl;

	cout << "2D Vector field access test:" << endl;
	displayVectorField(gridv);
	cout << endl;

	int i = VFXEpoch::RandomI(0, Ny - 1);
	int j = VFXEpoch::RandomI(0, Nx - 1);
	std::cout << '\n' << "The value of gridf at position" << "(" << i << ", " << j << ") is " << gridf(i, j) << '\n';
	std::cout << '\n' << "The value at gridv at position" << "(" << i << ", " << j << ") is " << "Vector2Df(" << gridv(i, j).m_x << ", " << gridv(i, j).m_y << ")" << '\n';
	cout << endl;

	return 0;
}