/**
 * @file
 */

#include "GLM.h"
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_access.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

/********************************************************/
/* AABB-triangle overlap test code                      */
/* by Tomas Akenine-Möller                              */
/* Function: int triBoxOverlap(float boxcenter[3],      */
/*          float boxhalfsize[3],float triverts[3][3]); */
/* History:                                             */
/*   2001-03-05: released the code in its first version */
/*   2001-06-18: changed the order of the tests, faster */
/*                                                      */
/* Acknowledgement: Many thanks to Pierre Terdiman for  */
/* suggestions and discussions on how to optimize code. */
/* Thanks to David Hunt for finding a ">="-bug!         */

/********************************************************/

/*
Copyright 2020 Tomas Akenine-Möller

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
documentation files (the "Software"), to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

namespace glm {

namespace priv {

static bool planeBoxOverlap(const glm::vec3 &normal, const glm::vec3 &vert, const glm::vec3 &maxbox) {
	int q;
	glm::vec3 vmin, vmax;
	for (q = 0; q <= 2; q++) {
		const float v = vert[q];
		if (normal[q] > 0.0f) {
			vmin[q] = -maxbox[q] - v;
			vmax[q] = maxbox[q] - v;
		} else {
			vmin[q] = maxbox[q] - v;
			vmax[q] = -maxbox[q] - v;
		}
	}

	if (glm::dot(normal, vmin) > 0.0f)
		return false;

	if (glm::dot(normal, vmax) >= 0.0f)
		return true;

	return false;
}

} // namespace priv

/*======================== X-tests ========================*/

#define AXISTEST_X01(a, b, fa, fb)                                                                                     \
	p0 = (a)*v0.y - (b)*v0.z;                                                                                          \
	p2 = (a)*v2.y - (b)*v2.z;                                                                                          \
	if (p0 < p2) {                                                                                                     \
		min = p0;                                                                                                      \
		max = p2;                                                                                                      \
	} else {                                                                                                           \
		min = p2;                                                                                                      \
		max = p0;                                                                                                      \
	}                                                                                                                  \
	rad = (fa)*boxhalfsize.y + (fb)*boxhalfsize.z;                                                                     \
	if (min > rad || max < -rad)                                                                                       \
		return false;

#define AXISTEST_X2(a, b, fa, fb)                                                                                      \
	p0 = (a)*v0.y - (b)*v0.z;                                                                                          \
	p1 = (a)*v1.y - (b)*v1.z;                                                                                          \
	if (p0 < p1) {                                                                                                     \
		min = p0;                                                                                                      \
		max = p1;                                                                                                      \
	} else {                                                                                                           \
		min = p1;                                                                                                      \
		max = p0;                                                                                                      \
	}                                                                                                                  \
	rad = (fa)*boxhalfsize.y + (fb)*boxhalfsize.z;                                                                     \
	if (min > rad || max < -rad)                                                                                       \
		return false;

/*======================== Y-tests ========================*/

#define AXISTEST_Y02(a, b, fa, fb)                                                                                     \
	p0 = -(a)*v0.x + (b)*v0.z;                                                                                         \
	p2 = -(a)*v2.x + (b)*v2.z;                                                                                         \
	if (p0 < p2) {                                                                                                     \
		min = p0;                                                                                                      \
		max = p2;                                                                                                      \
	} else {                                                                                                           \
		min = p2;                                                                                                      \
		max = p0;                                                                                                      \
	}                                                                                                                  \
	rad = (fa)*boxhalfsize.x + (fb)*boxhalfsize.z;                                                                     \
	if (min > rad || max < -rad)                                                                                       \
		return false;

#define AXISTEST_Y1(a, b, fa, fb)                                                                                      \
	p0 = -(a)*v0.x + (b)*v0.z;                                                                                         \
	p1 = -(a)*v1.x + (b)*v1.z;                                                                                         \
	if (p0 < p1) {                                                                                                     \
		min = p0;                                                                                                      \
		max = p1;                                                                                                      \
	} else {                                                                                                           \
		min = p1;                                                                                                      \
		max = p0;                                                                                                      \
	}                                                                                                                  \
	rad = (fa)*boxhalfsize.x + (fb)*boxhalfsize.z;                                                                     \
	if (min > rad || max < -rad)                                                                                       \
		return false;

/*======================== Z-tests ========================*/

#define AXISTEST_Z12(a, b, fa, fb)                                                                                     \
	p1 = (a)*v1.x - (b)*v1.y;                                                                                          \
	p2 = (a)*v2.x - (b)*v2.y;                                                                                          \
	if (p2 < p1) {                                                                                                     \
		min = p2;                                                                                                      \
		max = p1;                                                                                                      \
	} else {                                                                                                           \
		min = p1;                                                                                                      \
		max = p2;                                                                                                      \
	}                                                                                                                  \
	rad = (fa)*boxhalfsize.x + (fb)*boxhalfsize.y;                                                                     \
	if (min > rad || max < -rad)                                                                                       \
		return false;

#define AXISTEST_Z0(a, b, fa, fb)                                                                                      \
	p0 = (a)*v0.x - (b)*v0.y;                                                                                          \
	p1 = (a)*v1.x - (b)*v1.y;                                                                                          \
	if (p0 < p1) {                                                                                                     \
		min = p0;                                                                                                      \
		max = p1;                                                                                                      \
	} else {                                                                                                           \
		min = p1;                                                                                                      \
		max = p0;                                                                                                      \
	}                                                                                                                  \
	rad = (fa)*boxhalfsize.x + (fb)*boxhalfsize.y;                                                                     \
	if (min > rad || max < -rad)                                                                                       \
		return false;

#define FINDMINMAX(x0, x1, x2, min, max)                                                                               \
	(min) = (max) = (x0);                                                                                              \
	if ((x1) < (min))                                                                                                  \
		(min) = (x1);                                                                                                  \
	if ((x1) > (max))                                                                                                  \
		(max) = (x1);                                                                                                  \
	if ((x2) < (min))                                                                                                  \
		(min) = (x2);                                                                                                  \
	if ((x2) > (max))                                                                                                  \
		(max) = (x2);

// triBoxOverlap - see copyright above
bool intersectTriangleAABB(const glm::vec3 &boxcenter, const glm::vec3 &boxhalfsize, glm::vec3 v0, glm::vec3 v1,
						   glm::vec3 v2) {
	/*    use separating axis theorem to test overlap between triangle and box */
	/*    need to test for overlap in these directions: */
	/*    1) the {x,y,z}-directions (actually, since we use the AABB of the triangle */
	/*       we do not even need to test these) */
	/*    2) normal of the triangle */
	/*    3) crossproduct(edge from tri, {x,y,z}-direction) */
	/*       this gives 3x3=9 more tests */

	/* This is the fastest branch on Sun */

	/* move everything so that the boxcenter is in (0,0,0) */

	v0 -= boxcenter;
	v1 -= boxcenter;
	v2 -= boxcenter;

	/* compute triangle edges */
	const glm::vec3 e0 = v1 - v0; /* tri edge 0 */
	const glm::vec3 e1 = v2 - v1; /* tri edge 1 */
	const glm::vec3 e2 = v0 - v2; /* tri edge 2 */

	/* Bullet 3:  */

	/*  test the 9 tests first (this was faster) */
	float fex = glm::abs(e0.x);
	float fey = glm::abs(e0.y);
	float fez = glm::abs(e0.z);

	float p0, p1, p2, min, max, rad;

	AXISTEST_X01(e0.z, e0.y, fez, fey);
	AXISTEST_Y02(e0.z, e0.x, fez, fex);
	AXISTEST_Z12(e0.y, e0.x, fey, fex);

	fex = glm::abs(e1.x);
	fey = glm::abs(e1.y);
	fez = glm::abs(e1.z);

	AXISTEST_X01(e1.z, e1.y, fez, fey);
	AXISTEST_Y02(e1.z, e1.x, fez, fex);
	AXISTEST_Z0(e1.y, e1.x, fey, fex);

	fex = glm::abs(e2.x);
	fey = glm::abs(e2.y);
	fez = glm::abs(e2.z);

	AXISTEST_X2(e2.z, e2.y, fez, fey);
	AXISTEST_Y1(e2.z, e2.x, fez, fex);
	AXISTEST_Z12(e2.y, e2.x, fey, fex);

	/* Bullet 1: */

	/*  first test overlap in the {x,y,z}-directions */
	/*  find min, max of the triangle each direction, and test for overlap in */
	/*  that direction -- this is equivalent to testing a minimal AABB around */
	/*  the triangle against the AABB */

	/* test in X-direction */
	FINDMINMAX(v0.x, v1.x, v2.x, min, max);
	if (min > boxhalfsize.x || max < -boxhalfsize.x)
		return false;

	/* test in Y-direction */
	FINDMINMAX(v0.y, v1.y, v2.y, min, max);
	if (min > boxhalfsize.y || max < -boxhalfsize.y)
		return false;

	/* test in Z-direction */
	FINDMINMAX(v0.z, v1.z, v2.z, min, max);
	if (min > boxhalfsize.z || max < -boxhalfsize.z)
		return false;

	/* Bullet 2: */

	/*  test if the box intersects the plane of the triangle */
	/*  compute plane equation of triangle: normal*x+d=0 */
	return priv::planeBoxOverlap(glm::cross(e0, e1), v0, boxhalfsize);
}

bool intersectLines(const glm::vec3 &p1, const glm::vec3 &p2, const glm::vec3 &p3, const glm::vec3 &p4, glm::vec3 &pa,
					glm::vec3 &pb, float *pmua, float *pmub) {
	const glm::vec3 p13 = p1 - p3;
	const glm::vec3 p43 = p4 - p3;
	const vec3 eps(glm::epsilon<float>());
	if (glm::all(glm::lessThan(glm::abs(p43), eps))) {
		return false;
	}
	const glm::vec3 p21 = p2 - p1;
	if (glm::all(glm::lessThan(glm::abs(p21), eps))) {
		return false;
	}

	const float d1343 = glm::dot(p13, p43);
	const float d4321 = glm::dot(p43, p21);
	const float d1321 = glm::dot(p13, p21);
	const float d4343 = glm::dot(p43, p43);
	const float d2121 = glm::dot(p21, p21);

	const float denom = d2121 * d4343 - d4321 * d4321;
	if (glm::abs(denom) < glm::epsilon<float>()) {
		return false;
	}
	const float numer = d1343 * d4321 - d1321 * d4343;
	const float mua = numer / denom;
	const float mub = (d1343 + d4321 * mua) / d4343;

	pa = p1 + mua * p21;
	pb = p3 + mub * p43;

	if (pmua != nullptr) {
		*pmua = mua;
	}
	if (pmub != nullptr) {
		*pmub = mub;
	}

	return true;
}

vec3 transform(const mat4 &mat, const vec3 &v) {
	const mat4::col_type &c1 = column(mat, 0);
	const mat4::col_type &c2 = column(mat, 1);
	const mat4::col_type &c3 = column(mat, 2);
	vec3 r(c1.x * v.x + c1.y * v.y + c1.z * v.z + c1.w, c2.x * v.x + c2.y * v.y + c2.z * v.z + c2.w,
		   c3.x * v.x + c3.y * v.y + c3.z * v.z + c3.w);
	return r;
}

vec3 rotate(const mat4 &mat, const vec3 &v) {
	const vec3 c1 = column(mat, 0);
	const vec3 c2 = column(mat, 1);
	const vec3 c3 = column(mat, 2);
	return vec3(dot(c1, v), dot(c2, v), dot(c3, v));
}

vec3 project(const mat4 &m, const vec3 &p) {
	const vec4 &r = m * vec4(p, 1);
	return vec3(r) / r.w;
}

glm::mat3x4 invert(const glm::mat3x4 &o) {
	mat3x3 invrot(vec3(o[0].x, o[1].x, o[2].x), vec3(o[0].y, o[1].y, o[2].y), vec3(o[0].z, o[1].z, o[2].z));
	invrot[0] /= glm::length2(invrot[0]);
	invrot[1] /= glm::length2(invrot[1]);
	invrot[2] /= glm::length2(invrot[2]);
	const glm::vec3 trans(o[0].w, o[1].w, o[2].w);
	const glm::vec4 a(invrot[0], -dot(invrot[0], trans));
	const glm::vec4 b(invrot[1], -dot(invrot[1], trans));
	const glm::vec4 c(invrot[2], -dot(invrot[2], trans));
	return glm::mat3x4(a, b, c);
}

mat3x4 operator*(const mat3x4 &lhs, const mat3x4 &o) {
	glm::vec4 c1 = (o[0] * lhs[0].x + o[1] * lhs[0].y + o[2] * lhs[0].z);
	c1.w += lhs[0].w;
	glm::vec4 c2 = (o[0] * lhs[1].x + o[1] * lhs[1].y + o[2] * lhs[1].z);
	c2.w += lhs[1].w;
	glm::vec4 c3 = (o[0] * lhs[2].x + o[1] * lhs[2].y + o[2] * lhs[2].z);
	c3.w += lhs[2].w;
	return mat3x4(c1, c2, c3);
}

} // namespace glm
