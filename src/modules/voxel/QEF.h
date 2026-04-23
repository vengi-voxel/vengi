//----------------------------------------------------------------------------
// ThreeD Quadric Error Function
//----------------------------------------------------------------------------
#pragma once

#include <glm/fwd.hpp>

namespace voxel {

/**
 * QEF, implementing the quadric error function
 *      E[x] = P - Ni . Pi
 *
 * Given at least three points Pi, each with its respective
 * normal vector Ni, that describe at least two planes,
 * the QEF evalulates to the point x.
 */
glm::vec3 evaluateQEF(double mat[][3], double *vec, int rows); // TODO make these STL containers

// compute svd

void computeSVD(double mat[][3], // matrix (rows x 3)
				double u[][3],	 // matrix (rows x 3)
				double v[3][3],	 // matrix (3x3)
				double d[3],	 // vector (1x3)
				int rows);

// factorize

void factorize(double mat[][3], // matrix (rows x 3)
			   double tau_u[3], // vector (1x3)
			   double tau_v[2], // vectors, (1x2)
			   int rows);

double factorize_hh(double *ptrs[], int n);

// unpack

void unpack(double u[][3],	 // matrix (rows x 3)
			double v[3][3],	 // matrix (3x3)
			double tau_u[3], // vector, (1x3)
			double tau_v[2], // vector, (1x2)
			int rows);

// diagonalize

void diagonalize(double u[][3],	  // matrix (rows x 3)
				 double v[3][3],  // matrix (3x3)
				 double tau_u[3], // vector, (1x3)
				 double tau_v[2], // vector, (1x2)
				 int rows);

void chop(double *a, double *b, int n);

void qrstep(double u[][3],	// matrix (rows x cols)
			double v[][3],	// matrix (3 x cols)
			double tau_u[], // vector (1 x cols)
			double tau_v[], // vector (1 x cols - 1)
			int rows, int cols);

void qrstep_middle(double u[][3],  // matrix (rows x cols)
				   double tau_u[], // vector (1 x cols)
				   double tau_v[], // vector (1 x cols - 1)
				   int rows, int cols, int col);

void qrstep_end(double v[][3],	// matrix (3 x 3)
				double tau_u[], // vector (1 x 3)
				double tau_v[], // vector (1 x 2)
				int cols);

double qrstep_eigenvalue(double tau_u[], // vector (1 x 3)
						 double tau_v[], // vector (1 x 2)
						 int cols);

void qrstep_cols2(double u[][3],  // matrix (rows x 2)
				  double v[][3],  // matrix (3 x 2)
				  double tau_u[], // vector (1 x 2)
				  double tau_v[], // vector (1 x 1)
				  int rows);

void computeGivens(double a, double b, double *c, double *s);

void computeSchur(double a1, double a2, double a3, double *c, double *s);

// singularize

void singularize(double u[][3],	 // matrix (rows x 3)
				 double v[3][3], // matrix (3x3)
				 double d[3],	 // vector, (1x3)
				 int rows);

// solve svd
void solveSVD(double u[][3],  // matrix (rows x 3)
			  double v[3][3], // matrix (3x3)
			  double d[3],	  // vector (1x3)
			  double b[],	  // vector (1 x rows)
			  double x[3],	  // vector (1x3)
			  int rows);
} // namespace voxel
