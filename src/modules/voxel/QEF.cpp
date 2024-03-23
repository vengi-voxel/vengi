//----------------------------------------------------------------------------
// ThreeD Quadric Error Function
//----------------------------------------------------------------------------

#include "QEF.h"
#include "core/StandardLib.h"
#include <glm/common.hpp>
#include <glm/vec3.hpp>
#include <math.h>

namespace voxel {

//----------------------------------------------------------------------------

#define MAXROWS 12
#define EPSILON 1e-5

//----------------------------------------------------------------------------

glm::vec3 evaluateQEF(double mat[][3], double *vec, int rows) {
	// perform singular value decomposition on matrix mat
	// into u, v and d.
	// u is a matrix of rows x 3 (same as mat);
	// v is a square matrix 3 x 3 (for 3 columns in mat);
	// d is vector of 3 values representing the diagonal
	// matrix 3 x 3 (for 3 colums in mat).
	double u[MAXROWS][3], v[3][3], d[3];
	computeSVD(mat, u, v, d, rows);

	// solve linear system given by mat and vec using the
	// singular value decomposition of mat into u, v and d.
	if (d[2] < 0.1)
		d[2] = 0.0;
	if (d[1] < 0.1)
		d[1] = 0.0;
	if (d[0] < 0.1)
		d[0] = 0.0;

	double x[3];
	solveSVD(u, v, d, vec, x, rows);

	return {static_cast<float>(x[0]), static_cast<float>(x[1]), static_cast<float>(x[2])};
}

//----------------------------------------------------------------------------

void computeSVD(double mat[][3], // matrix (rows x 3)
				double u[][3],	 // matrix (rows x 3)
				double v[3][3],	 // matrix (3x3)
				double d[3],	 // vector (1x3)
				int rows) {
	core_memcpy(u, mat, rows * 3 * sizeof(double));

	double *tau_u = d;
	double tau_v[2];

	factorize(u, tau_u, tau_v, rows);

	unpack(u, v, tau_u, tau_v, rows);

	diagonalize(u, v, tau_u, tau_v, rows);

	singularize(u, v, tau_u, rows);
}

//----------------------------------------------------------------------------

void factorize(double mat[][3], // matrix (rows x 3)
			   double tau_u[3], // vector, (1x3)
			   double tau_v[2], // vector, (1x2)
			   int rows) {
	int y;

	// bidiagonal factorization of (rows x 3) matrix into :-
	// tau_u, a vector of 1x3 (for 3 columns in the matrix)
	// tau_v, a vector of 1x2 (one less column than the matrix)

	for (int i = 0; i < 3; ++i) {

		// set up a vector to reference into the matrix
		// from mat(i,i) to mat(m,i), that is, from the
		// i'th column of the i'th row and down all the way
		// through that column
		double *ptrs[MAXROWS];
		int num_ptrs = rows - i;
		for (int q = 0; q < num_ptrs; ++q)
			ptrs[q] = &mat[q + i][i];

		// perform householder transformation on this vector
		double tau = factorize_hh(ptrs, num_ptrs);
		tau_u[i] = tau;

		// all computations below this point are performed
		// only for the first two columns:  i=0 or i=1
		if (i + 1 < 3) {

			// perform householder transformation on the matrix
			// mat(i,i+1) to mat(m,n), that is, on the sub-matrix
			// that begins in the (i+1)'th column of the i'th
			// row and extends to the end of the matrix at (m,n)
			if (tau != 0.0) {
				for (int x = i + 1; x < 3; ++x) {
					double wx = mat[i][x];
					for (y = i + 1; y < rows; ++y)
						wx += mat[y][x] * (*ptrs[y - i]);
					double tau_wx = tau * wx;
					mat[i][x] -= tau_wx;
					for (y = i + 1; y < rows; ++y)
						mat[y][x] -= tau_wx * (*ptrs[y - i]);
				}
			}

			// perform householder transformation on i'th row
			// (remember at this point, i is either 0 or 1)

			// set up a vector to reference into the matrix
			// from mat(i,i+1) to mat(i,n), that is, from the
			// (i+1)'th column of the i'th row and all the way
			// through to the end of that row
			ptrs[0] = &mat[i][i + 1];
			if (i == 0) {
				ptrs[1] = &mat[i][i + 2];
				num_ptrs = 2;
			} else // i == 1
				num_ptrs = 1;

			// perform householder transformation on this vector
			tau = factorize_hh(ptrs, num_ptrs);
			tau_v[i] = tau;

			// perform householder transformation on the sub-matrix
			// mat(i+1,i+1) to mat(m,n), that is, on the sub-matrix
			// that begins in the (i+1)'th column of the (i+1)'th
			// row and extends to the end of the matrix at (m,n)
			if (tau != 0.0) {
				for (y = i + 1; y < rows; ++y) {
					double wy = mat[y][i + 1];
					if (i == 0)
						wy += mat[y][i + 2] * (*ptrs[1]);
					double tau_wy = tau * wy;
					mat[y][i + 1] -= tau_wy;
					if (i == 0)
						mat[y][i + 2] -= tau_wy * (*ptrs[1]);
				}
			}

		} // if (i + 1 < 3)
	}
}

//----------------------------------------------------------------------------

double factorize_hh(double *ptrs[], int n) {
	double tau = 0.0;

	if (n > 1) {
		double xnorm;
		if (n == 2)
			xnorm = glm::abs(*ptrs[1]);
		else {
			double scl = 0.0;
			double ssq = 1.0;
			for (int i = 1; i < n; ++i) {
				double x = glm::abs(*ptrs[i]);
				if (x != 0.0) {
					if (scl < x) {
						ssq = 1.0 + ssq * (scl / x) * (scl / x);
						scl = x;
					} else
						ssq += (x / scl) * (x / scl);
				}
			}
			xnorm = scl * sqrt(ssq);
		}

		if (xnorm != 0.0) {
			double alpha = *ptrs[0];
			double beta = sqrt(alpha * alpha + xnorm * xnorm);
			if (alpha >= 0.0)
				beta = -beta;
			tau = (beta - alpha) / beta;

			double scl = 1.0 / (alpha - beta);
			*ptrs[0] = beta;
			for (int i = 1; i < n; ++i)
				*ptrs[i] *= scl;
		}
	}

	return tau;
}

//----------------------------------------------------------------------------

void unpack(double u[][3],	 // matrix (rows x 3)
			double v[3][3],	 // matrix (3x3)
			double tau_u[3], // vector, (1x3)
			double tau_v[2], // vector, (1x2)
			int rows) {
	int i, y;

	// reset v to the identity matrix
	v[0][0] = v[1][1] = v[2][2] = 1.0;
	v[0][1] = v[0][2] = v[1][0] = v[1][2] = v[2][0] = v[2][1] = 0.0;

	for (i = 1; i >= 0; --i) {
		double tau = tau_v[i];

		// perform householder transformation on the sub-matrix
		// v(i+1,i+1) to v(m,n), that is, on the sub-matrix of v
		// that begins in the (i+1)'th column of the (i+1)'th row
		// and extends to the end of the matrix at (m,n).  the
		// householder vector used to perform this is the vector
		// from u(i,i+1) to u(i,n)
		if (tau != 0.0) {
			for (int x = i + 1; x < 3; ++x) {
				double wx = v[i + 1][x];
				for (y = i + 1 + 1; y < 3; ++y)
					wx += v[y][x] * u[i][y];
				double tau_wx = tau * wx;
				v[i + 1][x] -= tau_wx;
				for (y = i + 1 + 1; y < 3; ++y)
					v[y][x] -= tau_wx * u[i][y];
			}
		}
	}

	// copy superdiagonal of u into tau_v
	for (i = 0; i < 2; ++i)
		tau_v[i] = u[i][i + 1];

	// below, same idea for u:  householder transformations
	// and the superdiagonal copy

	for (i = 2; i >= 0; --i) {
		// copy superdiagonal of u into tau_u
		double tau = tau_u[i];
		tau_u[i] = u[i][i];

		// perform householder transformation on the sub-matrix
		// u(i,i) to u(m,n), that is, on the sub-matrix of u that
		// begins in the i'th column of the i'th row and extends
		// to the end of the matrix at (m,n).  the householder
		// vector used to perform this is the i'th column of u,
		// that is, u(0,i) to u(m,i)
		if (tau == 0.0) {
			u[i][i] = 1.0;
			if (i < 2) {
				u[i][2] = 0.0;
				if (i < 1)
					u[i][1] = 0.0;
			}
			for (y = i + 1; y < rows; ++y)
				u[y][i] = 0.0;
		} else {
			for (int x = i + 1; x < 3; ++x) {
				double wx = 0.0;
				for (y = i + 1; y < rows; ++y)
					wx += u[y][x] * u[y][i];
				double tau_wx = tau * wx;
				u[i][x] = -tau_wx;
				for (y = i + 1; y < rows; ++y)
					u[y][x] -= tau_wx * u[y][i];
			}
			for (y = i + 1; y < rows; ++y)
				u[y][i] = u[y][i] * -tau;
			u[i][i] = 1.0 - tau;
		}
	}
}

//----------------------------------------------------------------------------

void diagonalize(double u[][3],	  // matrix (rows x 3)
				 double v[3][3],  // matrix (3x3)
				 double tau_u[3], // vector, (1x3)
				 double tau_v[2], // vector, (1x2)
				 int rows) {
	int i, j;

	chop(tau_u, tau_v, 3);

	// progressively reduce the matrices into diagonal form

	int b = 3 - 1;
	while (b > 0) {
		if (tau_v[b - 1] == 0.0)
			--b;
		else {
			int a = b - 1;
			while (a > 0 && tau_v[a - 1] != 0.0)
				--a;
			int n = b - a + 1;

			double u1[MAXROWS][3];
			double v1[3][3];
			for (j = a; j <= b; ++j) {
				for (i = 0; i < rows; ++i)
					u1[i][j - a] = u[i][j];
				for (i = 0; i < 3; ++i)
					v1[i][j - a] = v[i][j];
			}

			qrstep(u1, v1, &tau_u[a], &tau_v[a], rows, n);

			for (j = a; j <= b; ++j) {
				for (i = 0; i < rows; ++i)
					u[i][j] = u1[i][j - a];
				for (i = 0; i < 3; ++i)
					v[i][j] = v1[i][j - a];
			}

			chop(&tau_u[a], &tau_v[a], n);
		}
	}
}

//----------------------------------------------------------------------------

void chop(double *a, double *b, int n) {
	double ai = a[0];
	for (int i = 0; i < n - 1; ++i) {
		double bi = b[i];
		double ai1 = a[i + 1];
		if (glm::abs(bi) < EPSILON * (glm::abs(ai) + glm::abs(ai1)))
			b[i] = 0.0;
		ai = ai1;
	}
}

//----------------------------------------------------------------------------

void qrstep(double u[][3],	// matrix (rows x cols)
			double v[][3],	// matrix (3 x cols)
			double tau_u[], // vector (1 x cols)
			double tau_v[], // vector (1 x cols - 1)
			int rows, int cols) {
	int i;

	if (cols == 2) {
		qrstep_cols2(u, v, tau_u, tau_v, rows);
		return;
	}

	// handle zeros on the diagonal or at its end
	for (i = 0; i < cols - 1; ++i)
		if (tau_u[i] == 0.0) {
			qrstep_middle(u, tau_u, tau_v, rows, cols, i);
			return;
		}
	if (tau_u[cols - 1] == 0.0) {
		qrstep_end(v, tau_u, tau_v, cols);
		return;
	}

	// perform qr reduction on the diagonal and off-diagonal

	double mu = qrstep_eigenvalue(tau_u, tau_v, cols);
	double y = tau_u[0] * tau_u[0] - mu;
	double z = tau_u[0] * tau_v[0];

	double ak = 0.0;
	double bk = 0.0;
	double zk;
	double ap = tau_u[0];
	double bp = tau_v[0];
	double aq = tau_u[1];
	//double bq = tau_v[1];

	for (int k = 0; k < cols - 1; ++k) {
		double c, s;

		// perform Givens rotation on V

		computeGivens(y, z, &c, &s);

		for (i = 0; i < 3; ++i) {
			double vip = v[i][k];
			double viq = v[i][k + 1];
			v[i][k] = vip * c - viq * s;
			v[i][k + 1] = vip * s + viq * c;
		}

		// perform Givens rotation on B

		double bk1 = bk * c - z * s;
		double ap1 = ap * c - bp * s;
		double bp1 = ap * s + bp * c;
		double zp1 = aq * -s;
		double aq1 = aq * c;

		if (k > 0)
			tau_v[k - 1] = bk1;

		ak = ap1;
		bk = bp1;
		zk = zp1;
		ap = aq1;

		if (k < cols - 2)
			bp = tau_v[k + 1];
		else
			bp = 0.0;

		y = ak;
		z = zk;

		// perform Givens rotation on U

		computeGivens(y, z, &c, &s);

		for (i = 0; i < rows; ++i) {
			double uip = u[i][k];
			double uiq = u[i][k + 1];
			u[i][k] = uip * c - uiq * s;
			u[i][k + 1] = uip * s + uiq * c;
		}

		// perform Givens rotation on B

		double ak1 = ak * c - zk * s;
		bk1 = bk * c - ap * s;
		double zk1 = bp * -s;

		ap1 = bk * s + ap * c;
		bp1 = bp * c;

		tau_u[k] = ak1;

		ak = ak1;
		bk = bk1;
		zk = zk1;
		ap = ap1;
		bp = bp1;

		if (k < cols - 2)
			aq = tau_u[k + 2];
		else
			aq = 0.0;

		y = bk;
		z = zk;
	}

	tau_v[cols - 2] = bk;
	tau_u[cols - 1] = ap;
}

//----------------------------------------------------------------------------

void qrstep_middle(double u[][3],  // matrix (rows x cols)
				   double tau_u[], // vector (1 x cols)
				   double tau_v[], // vector (1 x cols - 1)
				   int rows, int cols, int col) {
	double x = tau_v[col];
	double y = tau_u[col + 1];
	for (int j = col; j < cols - 1; ++j) {
		double c, s;

		// perform Givens rotation on U

		computeGivens(y, -x, &c, &s);
		for (int i = 0; i < rows; ++i) {
			double uip = u[i][col];
			double uiq = u[i][j + 1];
			u[i][col] = uip * c - uiq * s;
			u[i][j + 1] = uip * s + uiq * c;
		}

		// perform transposed Givens rotation on B

		tau_u[j + 1] = x * s + y * c;
		if (j == col)
			tau_v[j] = x * c - y * s;

		if (j < cols - 2) {
			double z = tau_v[j + 1];
			tau_v[j + 1] *= c;
			x = z * -s;
			y = tau_u[j + 2];
		}
	}
}

//----------------------------------------------------------------------------

void qrstep_end(double v[][3],	// matrix (3 x 3)
				double tau_u[], // vector (1 x 3)
				double tau_v[], // vector (1 x 2)
				int cols) {
	double x = tau_u[1];
	double y = tau_v[1];

	for (int k = 1; k >= 0; --k) {
		double c, s;

		// perform Givens rotation on V

		computeGivens(x, y, &c, &s);

		for (int i = 0; i < 3; ++i) {
			double vip = v[i][k];
			double viq = v[i][2];
			v[i][k] = vip * c - viq * s;
			v[i][2] = vip * s + viq * c;
		}

		// perform Givens rotation on B

		tau_u[k] = x * c - y * s;
		if (k == 1)
			tau_v[k] = x * s + y * c;
		if (k > 0) {
			double z = tau_v[k - 1];
			tau_v[k - 1] *= c;

			x = tau_u[k - 1];
			y = z * s;
		}
	}
}

//----------------------------------------------------------------------------

double qrstep_eigenvalue(double tau_u[], // vector (1 x 3)
						 double tau_v[], // vector (1 x 2)
						 int cols) {
	double ta = tau_u[1] * tau_u[1] + tau_v[0] * tau_v[0];
	double tb = tau_u[2] * tau_u[2] + tau_v[1] * tau_v[1];
	double tab = tau_u[1] * tau_v[1];
	double dt = (ta - tb) / 2.0;
	double mu;
	if (dt >= 0.0)
		mu = tb - (tab * tab) / (dt + sqrt(dt * dt + tab * tab));
	else
		mu = tb + (tab * tab) / (sqrt(dt * dt + tab * tab) - dt);
	return mu;
}

//----------------------------------------------------------------------------

void qrstep_cols2(double u[][3],  // matrix (rows x 2)
				  double v[][3],  // matrix (3 x 2)
				  double tau_u[], // vector (1 x 2)
				  double tau_v[], // vector (1 x 1)
				  int rows) {
	int i;
	double tmp;

	// eliminate off-diagonal element in [ 0  tau_v0 ]
	//                                   [ 0  tau_u1 ]
	// to make [ tau_u[0]  0 ]
	//         [ 0         0 ]

	if (tau_u[0] == 0.0) {
		double c, s;

		// perform transposed Givens rotation on B
		// multiplied by X = [ 0 1 ]
		//                   [ 1 0 ]

		computeGivens(tau_v[0], tau_u[1], &c, &s);

		tau_u[0] = tau_v[0] * c - tau_u[1] * s;
		tau_v[0] = tau_v[0] * s + tau_u[1] * c;
		tau_u[1] = 0.0;

		// perform Givens rotation on U

		for (i = 0; i < rows; ++i) {
			double uip = u[i][0];
			double uiq = u[i][1];
			u[i][0] = uip * c - uiq * s;
			u[i][1] = uip * s + uiq * c;
		}

		// multiply V by X, effectively swapping first two columns

		for (i = 0; i < 3; ++i) {
			tmp = v[i][0];
			v[i][0] = v[i][1];
			v[i][1] = tmp;
		}
	}

	// eliminate off-diagonal element in [ tau_u0  tau_v0 ]
	//                                   [ 0       0      ]

	else if (tau_u[1] == 0.0) {
		double c, s;

		// perform Givens rotation on B

		computeGivens(tau_u[0], tau_v[0], &c, &s);

		tau_u[0] = tau_u[0] * c - tau_v[0] * s;
		tau_v[0] = 0.0;

		// perform Givens rotation on V

		for (i = 0; i < 3; ++i) {
			double vip = v[i][0];
			double viq = v[i][1];
			v[i][0] = vip * c - viq * s;
			v[i][1] = vip * s + viq * c;
		}
	}

	// make colums orthogonal,

	else {
		double c, s;

		// perform Schur rotation on B

		computeSchur(tau_u[0], tau_v[0], tau_u[1], &c, &s);

		double a11 = tau_u[0] * c - tau_v[0] * s;
		double a21 = -tau_u[1] * s;
		double a12 = tau_u[0] * s + tau_v[0] * c;
		double a22 = tau_u[1] * c;

		// perform Schur rotation on V

		for (i = 0; i < 3; ++i) {
			double vip = v[i][0];
			double viq = v[i][1];
			v[i][0] = vip * c - viq * s;
			v[i][1] = vip * s + viq * c;
		}

		// eliminate off diagonal elements

		if ((a11 * a11 + a21 * a21) < (a12 * a12 + a22 * a22)) {

			// multiply B by X

			tmp = a11;
			a11 = a12;
			a12 = tmp;
			tmp = a21;
			a21 = a22;
			a22 = tmp;

			// multiply V by X, effectively swapping first
			// two columns

			for (i = 0; i < 3; ++i) {
				tmp = v[i][0];
				v[i][0] = v[i][1];
				v[i][1] = tmp;
			}
		}

		// perform transposed Givens rotation on B

		computeGivens(a11, a21, &c, &s);

		tau_u[0] = a11 * c - a21 * s;
		tau_v[0] = a12 * c - a22 * s;
		tau_u[1] = a12 * s + a22 * c;

		// perform Givens rotation on U

		for (i = 0; i < rows; ++i) {
			double uip = u[i][0];
			double uiq = u[i][1];
			u[i][0] = uip * c - uiq * s;
			u[i][1] = uip * s + uiq * c;
		}
	}
}

//----------------------------------------------------------------------------

void computeGivens(double a, double b, double *c, double *s) {
	if (b == 0.0) {
		*c = 1.0;
		*s = 0.0;
	} else if (glm::abs(b) > glm::abs(a)) {
		double t = -a / b;
		double s1 = 1.0 / sqrt(1 + t * t);
		*s = s1;
		*c = s1 * t;
	} else {
		double t = -b / a;
		double c1 = 1.0 / sqrt(1 + t * t);
		*c = c1;
		*s = c1 * t;
	}
}

//----------------------------------------------------------------------------

void computeSchur(double a1, double a2, double a3, double *c, double *s) {
	double apq = a1 * a2 * 2.0;

	if (apq == 0.0) {
		*c = 1.0;
		*s = 0.0;
	} else {
		double t;
		double tau = (a2 * a2 + (a3 + a1) * (a3 - a1)) / apq;
		if (tau >= 0.0)
			t = 1.0 / (tau + sqrt(1.0 + tau * tau));
		else
			t = -1.0 / (sqrt(1.0 + tau * tau) - tau);
		*c = 1.0 / sqrt(1.0 + t * t);
		*s = t * (*c);
	}
}

//----------------------------------------------------------------------------

void singularize(double u[][3],	 // matrix (rows x 3)
				 double v[3][3], // matrix (3x3)
				 double d[3],	 // vector, (1x3)
				 int rows) {
	int i, j, y;

	// make singularize values positive

	for (j = 0; j < 3; ++j)
		if (d[j] < 0.0) {
			for (i = 0; i < 3; ++i)
				v[i][j] = -v[i][j];
			d[j] = -d[j];
		}

	// sort singular values in decreasing order

	for (i = 0; i < 3; ++i) {
		double d_max = d[i];
		int i_max = i;
		for (j = i + 1; j < 3; ++j)
			if (d[j] > d_max) {
				d_max = d[j];
				i_max = j;
			}

		if (i_max != i) {
			// swap eigenvalues
			double tmp = d[i];
			d[i] = d[i_max];
			d[i_max] = tmp;

			// swap eigenvectors
			for (y = 0; y < rows; ++y) {
				tmp = u[y][i];
				u[y][i] = u[y][i_max];
				u[y][i_max] = tmp;
			}
			for (y = 0; y < 3; ++y) {
				tmp = v[y][i];
				v[y][i] = v[y][i_max];
				v[y][i_max] = tmp;
			}
		}
	}
}

//----------------------------------------------------------------------------

void solveSVD(double u[][3],  // matrix (rows x 3)
			  double v[3][3], // matrix (3x3)
			  double d[3],	  // vector (1x3)
			  double b[],	  // vector (1 x rows)
			  double x[3],	  // vector (1x3)
			  int rows) {
	static double zeroes[3] = {0.0, 0.0, 0.0};

	int i, j;

	// compute vector w = U^T * b

	double w[3];
	core_memcpy(w, zeroes, sizeof(w));
	for (i = 0; i < rows; ++i) {
		if (b[i] != 0.0)
			for (j = 0; j < 3; ++j)
				w[j] += b[i] * u[i][j];
	}

	// introduce non-zero singular values in d into w

	for (i = 0; i < 3; ++i) {
		if (d[i] != 0.0)
			w[i] /= d[i];
	}

	// compute result vector x = V * w

	for (i = 0; i < 3; ++i) {
		double tmp = 0.0;
		for (j = 0; j < 3; ++j)
			tmp += w[j] * v[i][j];
		x[i] = tmp;
	}
}

} // namespace voxel
