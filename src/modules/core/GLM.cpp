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

	float min;
	float max;
	float p0;
	float p1;
	float p2;
	float rad;

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
	if (min > rad || max < -rad) {
		return false;
	}

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
	if (min > rad || max < -rad) {
		return false;
	}

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
	if (min > rad || max < -rad) {
		return false;
	}

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
	if (min > rad || max < -rad) {
		return false;
	}

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
	if (min > rad || max < -rad) {
		return false;
	}

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
	if (min > rad || max < -rad) {
		return false;
	}

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
	if (min > rad || max < -rad) {
		return false;
	}

	/* Bullet 1: */
	/*  first test overlap in the {x,y,z}-directions */
	/*  find min, max of the triangle each direction, and test for overlap in */
	/*  that direction -- this is equivalent to testing a minimal AABB around */
	/*  the triangle against the AABB */

	/* test in X-direction */
	min = max = v0.x;
	if (v1.x < min) {
		min = v1.x;
	}
	if (v1.x > max) {
		max = v1.x;
	}
	if (v2.x < min) {
		min = v2.x;
	}
	if (v2.x > max) {
		max = v2.x;
	}
	if (min > boxhalfsize.x || max < -boxhalfsize.x) {
		return false;
	}

	/* test in Y-direction */
	min = max = v0.y;
	if (v1.y < min) {
		min = v1.y;
	}
	if (v1.y > max) {
		max = v1.y;
	}
	if (v2.y < min) {
		min = v2.y;
	}
	if (v2.y > max) {
		max = v2.y;
	}
	if (min > boxhalfsize.y || max < -boxhalfsize.y) {
		return false;
	}

	/* test in Z-direction */
	min = max = v0.z;
	if (v1.z < min) {
		min = v1.z;
	}
	if (v1.z > max) {
		max = v1.z;
	}
	if (v2.z < min) {
		min = v2.z;
	}
	if (v2.z > max) {
		max = v2.z;
	}
	if (min > boxhalfsize.z || max < -boxhalfsize.z) {
		return false;
	}

	/* Bullet 2: */
	/*  test if the box intersects the plane of the triangle */
	/*  compute plane equation of triangle: normal*x+d=0 */
	const glm::vec3& normal = glm::cross(e0, e1);

	glm::vec3 vmin;
	glm::vec3 vmax;
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
	if (glm::dot(normal, vmin) > 0.0f) {
		return false;
	}
	return glm::dot(normal, vmax) >= 0.0f;
}
float intersectLines(const glm::vec3& p1,
		const glm::vec3& p2, const glm::vec3& p3, const glm::vec3& p4,
		glm::vec3& pa, glm::vec3& pb, float *pmua = nullptr,
		float *pmub = nullptr) {
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

vec3 transform(const mat4& mat, const vec3& v) {
	const mat4::col_type& c1 = column(mat, 0);
	const mat4::col_type& c2 = column(mat, 1);
	const mat4::col_type& c3 = column(mat, 2);
	vec3 r(c1.x * v.x + c1.y * v.y + c1.z * v.z + c1.w,
		c2.x * v.x + c2.y * v.y + c2.z * v.z + c2.w,
		c3.x * v.x + c3.y * v.y + c3.z * v.z + c3.w);
	return r;
}

glm::vec3 transform(const mat3& mat, const glm::vec3& v) {
	const mat3::col_type& c1 = column(mat, 0);
	const mat3::col_type& c2 = column(mat, 1);
	const mat3::col_type& c3 = column(mat, 2);
	const vec3 r(dot(c1, v), dot(c2, v), dot(c3, v));
	return r;
}

glm::vec3 transform(const mat3x4& mat, const glm::vec3& v) {
	const mat3x4::col_type& c1 = column(mat, 0);
	const mat3x4::col_type& c2 = column(mat, 1);
	const mat3x4::col_type& c3 = column(mat, 2);
	const vec3 r(c1.x * v.x + c1.y * v.y + c1.z * v.z + c1.w,
		c2.x * v.x + c2.y * v.y + c2.z * v.z + c2.w,
		c3.x * v.x + c3.y * v.y + c3.z * v.z + c3.w);
	return r;
}

vec3 rotate(const mat4& mat, const vec3& v) {
	const mat4::col_type& c1 = column(mat, 0);
	const mat4::col_type& c2 = column(mat, 1);
	const mat4::col_type& c3 = column(mat, 2);
	const vec3 r(c1.x * v.x + c1.y * v.y + c1.z * v.z,
		c2.x * v.x + c2.y * v.y + c2.z * v.z,
		c3.x * v.x + c3.y * v.y + c3.z * v.z);
	return r;
}

vec3 project(const mat4& m, const vec3& p) {
	const vec4& r = m * vec4(p, 1);
	return vec3(r) / r.w;
}

glm::mat3x4 invert(const glm::mat3x4& o) {
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

mat3x4 operator*(const mat3x4& lhs, const mat3x4 &o) {
	glm::vec4 c1 = (o[0] * lhs[0].x + o[1] * lhs[0].y + o[2] * lhs[0].z);
	c1.w += lhs[0].w;
	glm::vec4 c2 = (o[0] * lhs[1].x + o[1] * lhs[1].y + o[2] * lhs[1].z);
	c2.w += lhs[1].w;
	glm::vec4 c3 = (o[0] * lhs[2].x + o[1] * lhs[2].y + o[2] * lhs[2].z);
	c3.w += lhs[2].w;
	return mat3x4(c1, c2, c3);
}

}

