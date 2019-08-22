#include "GLM.h"

namespace glm {

const vec3 forward  = vec3( 0.0f,  0.0f, -1.0f);
const vec3 backward = vec3( 0.0f,  0.0f,  1.0f);
const vec3 right    = vec3( 1.0f,  0.0f,  0.0f);
const vec3 left     = vec3(-1.0f,  0.0f,  0.0f);
const vec3 up       = vec3( 0.0f,  1.0f,  0.0f);
const vec3 down     = vec3( 0.0f, -1.0f,  0.0f);

/**
 * use separating axis theorem to test overlap between triangle and box
 * need to test for overlap in these directions:
 * 1) the {x,y,z}-directions (actually, since we use the AABB of the triangle
 *    we do not even need to test these)
 * 2) normal of the triangle
 * 3) crossproduct(edge from tri, {x,y,z}-directin)
 *    this gives 3x3=9 more tests
 */
bool intersectBoxTriangle(const glm::vec3& boxcenter, const glm::vec3& boxhalfsize, const glm::vec3& triv0, const glm::vec3& triv1, const glm::vec3& triv2) {
	/* move everything so that the boxcenter is in (0,0,0) */
	const glm::vec3& v0 = triv0 - boxcenter;
	const glm::vec3& v1 = triv1 - boxcenter;
	const glm::vec3& v2 = triv2 - boxcenter;

	/* compute triangle edges */
	const glm::vec3& e0 = v1 - v0; /* tri edge 0 */
	const glm::vec3& e1 = v2 - v1; /* tri edge 1 */
	const glm::vec3& e2 = v0 - v2; /* tri edge 2 */

	/* Bullet 3:  */
	/*  test the 9 tests first (this was faster) */
	float fex = glm::abs(e0.x);
	float fey = glm::abs(e0.y);
	float fez = glm::abs(e0.z);

	float min, max, p0, p1, p2, rad;

	p0 = e0.z * v0.y - e0.y * v0.z;
	p2 = e0.z * v2.y - e0.y * v2.z;
	if (p0 < p2) {
		min = p0;
		max = p2;
	} else {
		min = p2;
		max = p0;
	}
	rad = fez * boxhalfsize.y + fey * boxhalfsize.z;
	if (min > rad || max < -rad) {
		return false;
	}

	p0 = -e0.z * v0.x + e0.x * v0.z;
	p2 = -e0.z * v2.x + e0.x * v2.z;
	if (p0 < p2) {
		min = p0;
		max = p2;
	} else {
		min = p2;
		max = p0;
	}
	rad = fez * boxhalfsize.x + fex * boxhalfsize.z;
	if (min > rad || max < -rad) {
		return false;
	}

	p1 = e0.y * v1.x - e0.x * v1.y;
	p2 = e0.y * v2.x - e0.x * v2.y;
	if (p2 < p1) {
		min = p2;
		max = p1;
	} else {
		min = p1;
		max = p2;
	}
	rad = fey * boxhalfsize.x + fex * boxhalfsize.y;
	if (min > rad || max < -rad)
		return false;

	fex = glm::abs(e1.x);
	fey = glm::abs(e1.y);
	fez = glm::abs(e1.z);
	p0 = e1.z * v0.y - e1.y * v0.z;
	p2 = e1.z * v2.y - e1.y * v2.z;
	if (p0 < p2) {
		min = p0;
		max = p2;
	} else {
		min = p2;
		max = p0;
	}
	rad = fez * boxhalfsize.y + fey * boxhalfsize.z;
	if (min > rad || max < -rad)
		return false;

	p0 = -e1.z * v0.x + e1.x * v0.z;
	p2 = -e1.z * v2.x + e1.x * v2.z;
	if (p0 < p2) {
		min = p0;
		max = p2;
	} else {
		min = p2;
		max = p0;
	}
	rad = fez * boxhalfsize.x + fex * boxhalfsize.z;
	if (min > rad || max < -rad)
		return false;

	p0 = e1.y * v0.x - e1.x * v0.y;
	p1 = e1.y * v1.x - e1.x * v1.y;
	if (p0 < p1) {
		min = p0;
		max = p1;
	} else {
		min = p1;
		max = p0;
	}
	rad = fey * boxhalfsize.x + fex * boxhalfsize.y;
	if (min > rad || max < -rad)
		return false;

	fex = glm::abs(e2.x);
	fey = glm::abs(e2.y);
	fez = glm::abs(e2.z);
	p0 = e2.z * v0.y - e2.y * v0.z;
	p1 = e2.z * v1.y - e2.y * v1.z;
	if (p0 < p1) {
		min = p0;
		max = p1;
	} else {
		min = p1;
		max = p0;
	}
	rad = fez * boxhalfsize.y + fey * boxhalfsize.z;
	if (min > rad || max < -rad)
		return false;

	p0 = -e2.z * v0.x + e2.x * v0.z;
	p1 = -e2.z * v1.x + e2.x * v1.z;
	if (p0 < p1) {
		min = p0;
		max = p1;
	} else {
		min = p1;
		max = p0;
	}
	rad = fez * boxhalfsize.x + fex * boxhalfsize.z;
	if (min > rad || max < -rad)
		return false;

	p1 = e2.y * v1.x - e2.x * v1.y;
	p2 = e2.y * v2.x - e2.x * v2.y;
	if (p2 < p1) {
		min = p2;
		max = p1;
	} else {
		min = p1;
		max = p2;
	}
	rad = fey * boxhalfsize.x + fex * boxhalfsize.y;
	if (min > rad || max < -rad)
		return false;

	/* Bullet 1: */
	/*  first test overlap in the {x,y,z}-directions */
	/*  find min, max of the triangle each direction, and test for overlap in */
	/*  that direction -- this is equivalent to testing a minimal AABB around */
	/*  the triangle against the AABB */

	/* test in X-direction */
	min = max = v0.x;
	if (v1.x < min)
		min = v1.x;
	if (v1.x > max)
		max = v1.x;
	if (v2.x < min)
		min = v2.x;
	if (v2.x > max)
		max = v2.x;
	if (min > boxhalfsize.x || max < -boxhalfsize.x)
		return false;

	/* test in Y-direction */
	min = max = v0.y;
	if (v1.y < min)
		min = v1.y;
	if (v1.y > max)
		max = v1.y;
	if (v2.y < min)
		min = v2.y;
	if (v2.y > max)
		max = v2.y;
	if (min > boxhalfsize.y || max < -boxhalfsize.y) {
		return false;
	}

	/* test in Z-direction */
	min = max = v0.z;
	if (v1.z < min)
		min = v1.z;
	if (v1.z > max)
		max = v1.z;
	if (v2.z < min)
		min = v2.z;
	if (v2.z > max)
		max = v2.z;
	if (min > boxhalfsize.z || max < -boxhalfsize.z) {
		return false;
	}

	/* Bullet 2: */
	/*  test if the box intersects the plane of the triangle */
	/*  compute plane equation of triangle: normal*x+d=0 */
	const glm::vec3& normal = glm::cross(e0, e1);

	glm::vec3 vmin, vmax;
	for (int i = 0; i <= 2; i++) {
		const float v = v0[i];
		if (normal[i] > 0.0f) {
			vmin[i] = -boxhalfsize[i] - v;
			vmax[i] = boxhalfsize[i] - v;
		} else {
			vmin[i] = boxhalfsize[i] - v;
			vmax[i] = -boxhalfsize[i] - v;
		}
	}
	if (glm::dot(normal, vmin) > 0.0f)
		return false;
	if (glm::dot(normal, vmax) >= 0.0f)
		return true;

	return false;
}

}

