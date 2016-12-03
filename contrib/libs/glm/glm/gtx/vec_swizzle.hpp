/// @ref gtx_vec_swizzle
/// @file glm/gtx/vec_swizzle.hpp
///
/// @see core (dependence)
///
/// @defgroup gtx_vec_swizzle GLM_GTX_vec_swizzle
/// @ingroup gtx
///
/// @brief Functions to perform swizzle operation.
///
/// <glm/gtx/vec_swizzle.hpp> need to be included to use these functionalities.

#pragma once

#include "../glm.hpp"

#ifndef GLM_ENABLE_EXPERIMENTAL
#	error "GLM: GLM_GTX_vec_swizzle is an experimental extension and may change in the future. Use #define GLM_ENABLE_EXPERIMENTAL before including it, if you really want to use it."
#endif

namespace glm {
	// xx
	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> xx(const glm::tvec1<T, P> &v) {
		return glm::tvec2<T, P>(v.x, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> xx(const glm::tvec2<T, P> &v) {
		return glm::tvec2<T, P>(v.x, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> xx(const glm::tvec3<T, P> &v) {
		return glm::tvec2<T, P>(v.x, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> xx(const glm::tvec4<T, P> &v) {
		return glm::tvec2<T, P>(v.x, v.x);
	}

	// xy
	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> xy(const glm::tvec2<T, P> &v) {
		return glm::tvec2<T, P>(v.x, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> xy(const glm::tvec3<T, P> &v) {
		return glm::tvec2<T, P>(v.x, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> xy(const glm::tvec4<T, P> &v) {
		return glm::tvec2<T, P>(v.x, v.y);
	}

	// xz
	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> xz(const glm::tvec3<T, P> &v) {
		return glm::tvec2<T, P>(v.x, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> xz(const glm::tvec4<T, P> &v) {
		return glm::tvec2<T, P>(v.x, v.z);
	}

	// xw
	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> xw(const glm::tvec4<T, P> &v) {
		return glm::tvec2<T, P>(v.x, v.w);
	}

	// yx
	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> yx(const glm::tvec2<T, P> &v) {
		return glm::tvec2<T, P>(v.y, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> yx(const glm::tvec3<T, P> &v) {
		return glm::tvec2<T, P>(v.y, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> yx(const glm::tvec4<T, P> &v) {
		return glm::tvec2<T, P>(v.y, v.x);
	}

	// yy
	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> yy(const glm::tvec2<T, P> &v) {
		return glm::tvec2<T, P>(v.y, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> yy(const glm::tvec3<T, P> &v) {
		return glm::tvec2<T, P>(v.y, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> yy(const glm::tvec4<T, P> &v) {
		return glm::tvec2<T, P>(v.y, v.y);
	}

	// yz
	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> yz(const glm::tvec3<T, P> &v) {
		return glm::tvec2<T, P>(v.y, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> yz(const glm::tvec4<T, P> &v) {
		return glm::tvec2<T, P>(v.y, v.z);
	}

	// yw
	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> yw(const glm::tvec4<T, P> &v) {
		return glm::tvec2<T, P>(v.y, v.w);
	}

	// zx
	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> zx(const glm::tvec3<T, P> &v) {
		return glm::tvec2<T, P>(v.z, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> zx(const glm::tvec4<T, P> &v) {
		return glm::tvec2<T, P>(v.z, v.x);
	}

	// zy
	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> zy(const glm::tvec3<T, P> &v) {
		return glm::tvec2<T, P>(v.z, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> zy(const glm::tvec4<T, P> &v) {
		return glm::tvec2<T, P>(v.z, v.y);
	}

	// zz
	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> zz(const glm::tvec3<T, P> &v) {
		return glm::tvec2<T, P>(v.z, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> zz(const glm::tvec4<T, P> &v) {
		return glm::tvec2<T, P>(v.z, v.z);
	}

	// zw
	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> zw(const glm::tvec4<T, P> &v) {
		return glm::tvec2<T, P>(v.z, v.w);
	}

	// wx
	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> wx(const glm::tvec4<T, P> &v) {
		return glm::tvec2<T, P>(v.w, v.x);
	}

	// wy
	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> wy(const glm::tvec4<T, P> &v) {
		return glm::tvec2<T, P>(v.w, v.y);
	}

	// wz
	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> wz(const glm::tvec4<T, P> &v) {
		return glm::tvec2<T, P>(v.w, v.z);
	}

	// ww
	template<typename T, precision P>
	GLM_INLINE glm::tvec2<T, P> ww(const glm::tvec4<T, P> &v) {
		return glm::tvec2<T, P>(v.w, v.w);
	}

	// xxx
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xxx(const glm::tvec1<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.x, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xxx(const glm::tvec2<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.x, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xxx(const glm::tvec3<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.x, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xxx(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.x, v.x);
	}

	// xxy
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xxy(const glm::tvec2<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.x, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xxy(const glm::tvec3<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.x, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xxy(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.x, v.y);
	}

	// xxz
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xxz(const glm::tvec3<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.x, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xxz(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.x, v.z);
	}

	// xxw
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xxw(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.x, v.w);
	}

	// xyx
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xyx(const glm::tvec2<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.y, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xyx(const glm::tvec3<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.y, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xyx(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.y, v.x);
	}

	// xyy
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xyy(const glm::tvec2<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.y, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xyy(const glm::tvec3<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.y, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xyy(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.y, v.y);
	}

	// xyz
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xyz(const glm::tvec3<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.y, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xyz(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.y, v.z);
	}

	// xyw
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xyw(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.y, v.w);
	}

	// xzx
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xzx(const glm::tvec3<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.z, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xzx(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.z, v.x);
	}

	// xzy
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xzy(const glm::tvec3<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.z, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xzy(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.z, v.y);
	}

	// xzz
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xzz(const glm::tvec3<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.z, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xzz(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.z, v.z);
	}

	// xzw
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xzw(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.z, v.w);
	}

	// xwx
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xwx(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.w, v.x);
	}

	// xwy
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xwy(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.w, v.y);
	}

	// xwz
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xwz(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.w, v.z);
	}

	// xww
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> xww(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.x, v.w, v.w);
	}

	// yxx
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> yxx(const glm::tvec2<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.x, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> yxx(const glm::tvec3<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.x, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> yxx(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.x, v.x);
	}

	// yxy
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> yxy(const glm::tvec2<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.x, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> yxy(const glm::tvec3<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.x, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> yxy(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.x, v.y);
	}

	// yxz
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> yxz(const glm::tvec3<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.x, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> yxz(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.x, v.z);
	}

	// yxw
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> yxw(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.x, v.w);
	}

	// yyx
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> yyx(const glm::tvec2<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.y, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> yyx(const glm::tvec3<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.y, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> yyx(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.y, v.x);
	}

	// yyy
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> yyy(const glm::tvec2<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.y, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> yyy(const glm::tvec3<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.y, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> yyy(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.y, v.y);
	}

	// yyz
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> yyz(const glm::tvec3<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.y, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> yyz(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.y, v.z);
	}

	// yyw
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> yyw(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.y, v.w);
	}

	// yzx
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> yzx(const glm::tvec3<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.z, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> yzx(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.z, v.x);
	}

	// yzy
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> yzy(const glm::tvec3<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.z, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> yzy(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.z, v.y);
	}

	// yzz
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> yzz(const glm::tvec3<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.z, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> yzz(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.z, v.z);
	}

	// yzw
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> yzw(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.z, v.w);
	}

	// ywx
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> ywx(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.w, v.x);
	}

	// ywy
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> ywy(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.w, v.y);
	}

	// ywz
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> ywz(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.w, v.z);
	}

	// yww
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> yww(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.y, v.w, v.w);
	}

	// zxx
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> zxx(const glm::tvec3<T, P> &v) {
		return glm::tvec3<T, P>(v.z, v.x, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> zxx(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.z, v.x, v.x);
	}

	// zxy
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> zxy(const glm::tvec3<T, P> &v) {
		return glm::tvec3<T, P>(v.z, v.x, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> zxy(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.z, v.x, v.y);
	}

	// zxz
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> zxz(const glm::tvec3<T, P> &v) {
		return glm::tvec3<T, P>(v.z, v.x, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> zxz(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.z, v.x, v.z);
	}

	// zxw
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> zxw(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.z, v.x, v.w);
	}

	// zyx
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> zyx(const glm::tvec3<T, P> &v) {
		return glm::tvec3<T, P>(v.z, v.y, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> zyx(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.z, v.y, v.x);
	}

	// zyy
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> zyy(const glm::tvec3<T, P> &v) {
		return glm::tvec3<T, P>(v.z, v.y, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> zyy(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.z, v.y, v.y);
	}

	// zyz
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> zyz(const glm::tvec3<T, P> &v) {
		return glm::tvec3<T, P>(v.z, v.y, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> zyz(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.z, v.y, v.z);
	}

	// zyw
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> zyw(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.z, v.y, v.w);
	}

	// zzx
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> zzx(const glm::tvec3<T, P> &v) {
		return glm::tvec3<T, P>(v.z, v.z, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> zzx(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.z, v.z, v.x);
	}

	// zzy
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> zzy(const glm::tvec3<T, P> &v) {
		return glm::tvec3<T, P>(v.z, v.z, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> zzy(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.z, v.z, v.y);
	}

	// zzz
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> zzz(const glm::tvec3<T, P> &v) {
		return glm::tvec3<T, P>(v.z, v.z, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> zzz(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.z, v.z, v.z);
	}

	// zzw
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> zzw(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.z, v.z, v.w);
	}

	// zwx
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> zwx(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.z, v.w, v.x);
	}

	// zwy
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> zwy(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.z, v.w, v.y);
	}

	// zwz
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> zwz(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.z, v.w, v.z);
	}

	// zww
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> zww(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.z, v.w, v.w);
	}

	// wxx
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> wxx(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.w, v.x, v.x);
	}

	// wxy
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> wxy(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.w, v.x, v.y);
	}

	// wxz
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> wxz(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.w, v.x, v.z);
	}

	// wxw
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> wxw(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.w, v.x, v.w);
	}

	// wyx
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> wyx(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.w, v.y, v.x);
	}

	// wyy
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> wyy(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.w, v.y, v.y);
	}

	// wyz
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> wyz(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.w, v.y, v.z);
	}

	// wyw
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> wyw(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.w, v.y, v.w);
	}

	// wzx
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> wzx(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.w, v.z, v.x);
	}

	// wzy
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> wzy(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.w, v.z, v.y);
	}

	// wzz
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> wzz(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.w, v.z, v.z);
	}

	// wzw
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> wzw(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.w, v.z, v.w);
	}

	// wwx
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> wwx(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.w, v.w, v.x);
	}

	// wwy
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> wwy(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.w, v.w, v.y);
	}

	// wwz
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> wwz(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.w, v.w, v.z);
	}

	// www
	template<typename T, precision P>
	GLM_INLINE glm::tvec3<T, P> www(const glm::tvec4<T, P> &v) {
		return glm::tvec3<T, P>(v.w, v.w, v.w);
	}

	// xxxx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxxx(const glm::tvec1<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.x, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxxx(const glm::tvec2<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.x, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxxx(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.x, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxxx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.x, v.x);
	}

	// xxxy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxxy(const glm::tvec2<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.x, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxxy(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.x, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxxy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.x, v.y);
	}

	// xxxz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxxz(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.x, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxxz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.x, v.z);
	}

	// xxxw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxxw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.x, v.w);
	}

	// xxyx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxyx(const glm::tvec2<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.y, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxyx(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.y, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxyx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.y, v.x);
	}

	// xxyy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxyy(const glm::tvec2<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.y, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxyy(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.y, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxyy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.y, v.y);
	}

	// xxyz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxyz(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.y, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxyz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.y, v.z);
	}

	// xxyw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxyw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.y, v.w);
	}

	// xxzx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxzx(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.z, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxzx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.z, v.x);
	}

	// xxzy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxzy(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.z, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxzy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.z, v.y);
	}

	// xxzz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxzz(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.z, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxzz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.z, v.z);
	}

	// xxzw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxzw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.z, v.w);
	}

	// xxwx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxwx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.w, v.x);
	}

	// xxwy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxwy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.w, v.y);
	}

	// xxwz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxwz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.w, v.z);
	}

	// xxww
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xxww(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.x, v.w, v.w);
	}

	// xyxx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xyxx(const glm::tvec2<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.x, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xyxx(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.x, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xyxx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.x, v.x);
	}

	// xyxy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xyxy(const glm::tvec2<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.x, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xyxy(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.x, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xyxy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.x, v.y);
	}

	// xyxz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xyxz(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.x, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xyxz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.x, v.z);
	}

	// xyxw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xyxw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.x, v.w);
	}

	// xyyx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xyyx(const glm::tvec2<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.y, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xyyx(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.y, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xyyx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.y, v.x);
	}

	// xyyy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xyyy(const glm::tvec2<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.y, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xyyy(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.y, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xyyy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.y, v.y);
	}

	// xyyz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xyyz(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.y, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xyyz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.y, v.z);
	}

	// xyyw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xyyw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.y, v.w);
	}

	// xyzx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xyzx(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.z, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xyzx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.z, v.x);
	}

	// xyzy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xyzy(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.z, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xyzy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.z, v.y);
	}

	// xyzz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xyzz(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.z, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xyzz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.z, v.z);
	}

	// xyzw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xyzw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.z, v.w);
	}

	// xywx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xywx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.w, v.x);
	}

	// xywy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xywy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.w, v.y);
	}

	// xywz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xywz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.w, v.z);
	}

	// xyww
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xyww(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.y, v.w, v.w);
	}

	// xzxx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xzxx(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.z, v.x, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xzxx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.z, v.x, v.x);
	}

	// xzxy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xzxy(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.z, v.x, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xzxy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.z, v.x, v.y);
	}

	// xzxz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xzxz(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.z, v.x, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xzxz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.z, v.x, v.z);
	}

	// xzxw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xzxw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.z, v.x, v.w);
	}

	// xzyx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xzyx(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.z, v.y, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xzyx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.z, v.y, v.x);
	}

	// xzyy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xzyy(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.z, v.y, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xzyy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.z, v.y, v.y);
	}

	// xzyz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xzyz(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.z, v.y, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xzyz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.z, v.y, v.z);
	}

	// xzyw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xzyw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.z, v.y, v.w);
	}

	// xzzx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xzzx(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.z, v.z, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xzzx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.z, v.z, v.x);
	}

	// xzzy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xzzy(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.z, v.z, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xzzy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.z, v.z, v.y);
	}

	// xzzz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xzzz(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.z, v.z, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xzzz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.z, v.z, v.z);
	}

	// xzzw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xzzw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.z, v.z, v.w);
	}

	// xzwx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xzwx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.z, v.w, v.x);
	}

	// xzwy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xzwy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.z, v.w, v.y);
	}

	// xzwz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xzwz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.z, v.w, v.z);
	}

	// xzww
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xzww(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.z, v.w, v.w);
	}

	// xwxx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xwxx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.w, v.x, v.x);
	}

	// xwxy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xwxy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.w, v.x, v.y);
	}

	// xwxz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xwxz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.w, v.x, v.z);
	}

	// xwxw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xwxw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.w, v.x, v.w);
	}

	// xwyx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xwyx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.w, v.y, v.x);
	}

	// xwyy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xwyy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.w, v.y, v.y);
	}

	// xwyz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xwyz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.w, v.y, v.z);
	}

	// xwyw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xwyw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.w, v.y, v.w);
	}

	// xwzx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xwzx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.w, v.z, v.x);
	}

	// xwzy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xwzy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.w, v.z, v.y);
	}

	// xwzz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xwzz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.w, v.z, v.z);
	}

	// xwzw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xwzw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.w, v.z, v.w);
	}

	// xwwx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xwwx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.w, v.w, v.x);
	}

	// xwwy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xwwy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.w, v.w, v.y);
	}

	// xwwz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xwwz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.w, v.w, v.z);
	}

	// xwww
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> xwww(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.x, v.w, v.w, v.w);
	}

	// yxxx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxxx(const glm::tvec2<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.x, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxxx(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.x, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxxx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.x, v.x);
	}

	// yxxy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxxy(const glm::tvec2<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.x, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxxy(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.x, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxxy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.x, v.y);
	}

	// yxxz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxxz(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.x, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxxz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.x, v.z);
	}

	// yxxw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxxw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.x, v.w);
	}

	// yxyx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxyx(const glm::tvec2<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.y, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxyx(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.y, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxyx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.y, v.x);
	}

	// yxyy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxyy(const glm::tvec2<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.y, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxyy(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.y, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxyy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.y, v.y);
	}

	// yxyz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxyz(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.y, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxyz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.y, v.z);
	}

	// yxyw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxyw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.y, v.w);
	}

	// yxzx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxzx(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.z, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxzx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.z, v.x);
	}

	// yxzy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxzy(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.z, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxzy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.z, v.y);
	}

	// yxzz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxzz(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.z, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxzz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.z, v.z);
	}

	// yxzw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxzw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.z, v.w);
	}

	// yxwx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxwx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.w, v.x);
	}

	// yxwy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxwy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.w, v.y);
	}

	// yxwz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxwz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.w, v.z);
	}

	// yxww
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yxww(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.x, v.w, v.w);
	}

	// yyxx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yyxx(const glm::tvec2<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.x, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yyxx(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.x, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yyxx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.x, v.x);
	}

	// yyxy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yyxy(const glm::tvec2<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.x, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yyxy(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.x, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yyxy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.x, v.y);
	}

	// yyxz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yyxz(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.x, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yyxz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.x, v.z);
	}

	// yyxw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yyxw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.x, v.w);
	}

	// yyyx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yyyx(const glm::tvec2<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.y, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yyyx(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.y, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yyyx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.y, v.x);
	}

	// yyyy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yyyy(const glm::tvec2<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.y, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yyyy(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.y, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yyyy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.y, v.y);
	}

	// yyyz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yyyz(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.y, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yyyz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.y, v.z);
	}

	// yyyw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yyyw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.y, v.w);
	}

	// yyzx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yyzx(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.z, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yyzx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.z, v.x);
	}

	// yyzy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yyzy(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.z, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yyzy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.z, v.y);
	}

	// yyzz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yyzz(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.z, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yyzz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.z, v.z);
	}

	// yyzw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yyzw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.z, v.w);
	}

	// yywx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yywx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.w, v.x);
	}

	// yywy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yywy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.w, v.y);
	}

	// yywz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yywz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.w, v.z);
	}

	// yyww
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yyww(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.y, v.w, v.w);
	}

	// yzxx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yzxx(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.z, v.x, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yzxx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.z, v.x, v.x);
	}

	// yzxy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yzxy(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.z, v.x, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yzxy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.z, v.x, v.y);
	}

	// yzxz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yzxz(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.z, v.x, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yzxz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.z, v.x, v.z);
	}

	// yzxw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yzxw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.z, v.x, v.w);
	}

	// yzyx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yzyx(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.z, v.y, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yzyx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.z, v.y, v.x);
	}

	// yzyy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yzyy(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.z, v.y, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yzyy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.z, v.y, v.y);
	}

	// yzyz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yzyz(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.z, v.y, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yzyz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.z, v.y, v.z);
	}

	// yzyw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yzyw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.z, v.y, v.w);
	}

	// yzzx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yzzx(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.z, v.z, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yzzx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.z, v.z, v.x);
	}

	// yzzy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yzzy(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.z, v.z, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yzzy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.z, v.z, v.y);
	}

	// yzzz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yzzz(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.z, v.z, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yzzz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.z, v.z, v.z);
	}

	// yzzw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yzzw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.z, v.z, v.w);
	}

	// yzwx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yzwx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.z, v.w, v.x);
	}

	// yzwy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yzwy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.z, v.w, v.y);
	}

	// yzwz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yzwz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.z, v.w, v.z);
	}

	// yzww
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> yzww(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.z, v.w, v.w);
	}

	// ywxx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> ywxx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.w, v.x, v.x);
	}

	// ywxy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> ywxy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.w, v.x, v.y);
	}

	// ywxz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> ywxz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.w, v.x, v.z);
	}

	// ywxw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> ywxw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.w, v.x, v.w);
	}

	// ywyx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> ywyx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.w, v.y, v.x);
	}

	// ywyy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> ywyy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.w, v.y, v.y);
	}

	// ywyz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> ywyz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.w, v.y, v.z);
	}

	// ywyw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> ywyw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.w, v.y, v.w);
	}

	// ywzx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> ywzx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.w, v.z, v.x);
	}

	// ywzy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> ywzy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.w, v.z, v.y);
	}

	// ywzz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> ywzz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.w, v.z, v.z);
	}

	// ywzw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> ywzw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.w, v.z, v.w);
	}

	// ywwx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> ywwx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.w, v.w, v.x);
	}

	// ywwy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> ywwy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.w, v.w, v.y);
	}

	// ywwz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> ywwz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.w, v.w, v.z);
	}

	// ywww
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> ywww(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.y, v.w, v.w, v.w);
	}

	// zxxx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zxxx(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.x, v.x, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zxxx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.x, v.x, v.x);
	}

	// zxxy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zxxy(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.x, v.x, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zxxy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.x, v.x, v.y);
	}

	// zxxz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zxxz(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.x, v.x, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zxxz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.x, v.x, v.z);
	}

	// zxxw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zxxw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.x, v.x, v.w);
	}

	// zxyx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zxyx(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.x, v.y, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zxyx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.x, v.y, v.x);
	}

	// zxyy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zxyy(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.x, v.y, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zxyy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.x, v.y, v.y);
	}

	// zxyz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zxyz(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.x, v.y, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zxyz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.x, v.y, v.z);
	}

	// zxyw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zxyw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.x, v.y, v.w);
	}

	// zxzx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zxzx(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.x, v.z, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zxzx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.x, v.z, v.x);
	}

	// zxzy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zxzy(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.x, v.z, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zxzy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.x, v.z, v.y);
	}

	// zxzz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zxzz(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.x, v.z, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zxzz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.x, v.z, v.z);
	}

	// zxzw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zxzw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.x, v.z, v.w);
	}

	// zxwx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zxwx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.x, v.w, v.x);
	}

	// zxwy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zxwy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.x, v.w, v.y);
	}

	// zxwz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zxwz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.x, v.w, v.z);
	}

	// zxww
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zxww(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.x, v.w, v.w);
	}

	// zyxx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zyxx(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.y, v.x, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zyxx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.y, v.x, v.x);
	}

	// zyxy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zyxy(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.y, v.x, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zyxy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.y, v.x, v.y);
	}

	// zyxz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zyxz(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.y, v.x, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zyxz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.y, v.x, v.z);
	}

	// zyxw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zyxw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.y, v.x, v.w);
	}

	// zyyx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zyyx(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.y, v.y, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zyyx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.y, v.y, v.x);
	}

	// zyyy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zyyy(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.y, v.y, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zyyy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.y, v.y, v.y);
	}

	// zyyz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zyyz(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.y, v.y, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zyyz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.y, v.y, v.z);
	}

	// zyyw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zyyw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.y, v.y, v.w);
	}

	// zyzx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zyzx(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.y, v.z, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zyzx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.y, v.z, v.x);
	}

	// zyzy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zyzy(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.y, v.z, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zyzy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.y, v.z, v.y);
	}

	// zyzz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zyzz(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.y, v.z, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zyzz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.y, v.z, v.z);
	}

	// zyzw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zyzw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.y, v.z, v.w);
	}

	// zywx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zywx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.y, v.w, v.x);
	}

	// zywy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zywy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.y, v.w, v.y);
	}

	// zywz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zywz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.y, v.w, v.z);
	}

	// zyww
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zyww(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.y, v.w, v.w);
	}

	// zzxx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zzxx(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.z, v.x, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zzxx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.z, v.x, v.x);
	}

	// zzxy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zzxy(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.z, v.x, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zzxy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.z, v.x, v.y);
	}

	// zzxz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zzxz(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.z, v.x, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zzxz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.z, v.x, v.z);
	}

	// zzxw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zzxw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.z, v.x, v.w);
	}

	// zzyx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zzyx(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.z, v.y, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zzyx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.z, v.y, v.x);
	}

	// zzyy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zzyy(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.z, v.y, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zzyy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.z, v.y, v.y);
	}

	// zzyz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zzyz(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.z, v.y, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zzyz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.z, v.y, v.z);
	}

	// zzyw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zzyw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.z, v.y, v.w);
	}

	// zzzx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zzzx(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.z, v.z, v.x);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zzzx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.z, v.z, v.x);
	}

	// zzzy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zzzy(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.z, v.z, v.y);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zzzy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.z, v.z, v.y);
	}

	// zzzz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zzzz(const glm::tvec3<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.z, v.z, v.z);
	}

	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zzzz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.z, v.z, v.z);
	}

	// zzzw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zzzw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.z, v.z, v.w);
	}

	// zzwx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zzwx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.z, v.w, v.x);
	}

	// zzwy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zzwy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.z, v.w, v.y);
	}

	// zzwz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zzwz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.z, v.w, v.z);
	}

	// zzww
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zzww(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.z, v.w, v.w);
	}

	// zwxx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zwxx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.w, v.x, v.x);
	}

	// zwxy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zwxy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.w, v.x, v.y);
	}

	// zwxz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zwxz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.w, v.x, v.z);
	}

	// zwxw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zwxw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.w, v.x, v.w);
	}

	// zwyx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zwyx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.w, v.y, v.x);
	}

	// zwyy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zwyy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.w, v.y, v.y);
	}

	// zwyz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zwyz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.w, v.y, v.z);
	}

	// zwyw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zwyw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.w, v.y, v.w);
	}

	// zwzx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zwzx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.w, v.z, v.x);
	}

	// zwzy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zwzy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.w, v.z, v.y);
	}

	// zwzz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zwzz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.w, v.z, v.z);
	}

	// zwzw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zwzw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.w, v.z, v.w);
	}

	// zwwx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zwwx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.w, v.w, v.x);
	}

	// zwwy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zwwy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.w, v.w, v.y);
	}

	// zwwz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zwwz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.w, v.w, v.z);
	}

	// zwww
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> zwww(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.z, v.w, v.w, v.w);
	}

	// wxxx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wxxx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.x, v.x, v.x);
	}

	// wxxy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wxxy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.x, v.x, v.y);
	}

	// wxxz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wxxz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.x, v.x, v.z);
	}

	// wxxw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wxxw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.x, v.x, v.w);
	}

	// wxyx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wxyx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.x, v.y, v.x);
	}

	// wxyy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wxyy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.x, v.y, v.y);
	}

	// wxyz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wxyz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.x, v.y, v.z);
	}

	// wxyw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wxyw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.x, v.y, v.w);
	}

	// wxzx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wxzx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.x, v.z, v.x);
	}

	// wxzy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wxzy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.x, v.z, v.y);
	}

	// wxzz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wxzz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.x, v.z, v.z);
	}

	// wxzw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wxzw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.x, v.z, v.w);
	}

	// wxwx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wxwx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.x, v.w, v.x);
	}

	// wxwy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wxwy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.x, v.w, v.y);
	}

	// wxwz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wxwz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.x, v.w, v.z);
	}

	// wxww
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wxww(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.x, v.w, v.w);
	}

	// wyxx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wyxx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.y, v.x, v.x);
	}

	// wyxy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wyxy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.y, v.x, v.y);
	}

	// wyxz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wyxz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.y, v.x, v.z);
	}

	// wyxw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wyxw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.y, v.x, v.w);
	}

	// wyyx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wyyx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.y, v.y, v.x);
	}

	// wyyy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wyyy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.y, v.y, v.y);
	}

	// wyyz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wyyz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.y, v.y, v.z);
	}

	// wyyw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wyyw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.y, v.y, v.w);
	}

	// wyzx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wyzx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.y, v.z, v.x);
	}

	// wyzy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wyzy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.y, v.z, v.y);
	}

	// wyzz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wyzz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.y, v.z, v.z);
	}

	// wyzw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wyzw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.y, v.z, v.w);
	}

	// wywx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wywx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.y, v.w, v.x);
	}

	// wywy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wywy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.y, v.w, v.y);
	}

	// wywz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wywz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.y, v.w, v.z);
	}

	// wyww
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wyww(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.y, v.w, v.w);
	}

	// wzxx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wzxx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.z, v.x, v.x);
	}

	// wzxy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wzxy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.z, v.x, v.y);
	}

	// wzxz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wzxz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.z, v.x, v.z);
	}

	// wzxw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wzxw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.z, v.x, v.w);
	}

	// wzyx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wzyx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.z, v.y, v.x);
	}

	// wzyy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wzyy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.z, v.y, v.y);
	}

	// wzyz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wzyz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.z, v.y, v.z);
	}

	// wzyw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wzyw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.z, v.y, v.w);
	}

	// wzzx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wzzx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.z, v.z, v.x);
	}

	// wzzy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wzzy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.z, v.z, v.y);
	}

	// wzzz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wzzz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.z, v.z, v.z);
	}

	// wzzw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wzzw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.z, v.z, v.w);
	}

	// wzwx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wzwx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.z, v.w, v.x);
	}

	// wzwy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wzwy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.z, v.w, v.y);
	}

	// wzwz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wzwz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.z, v.w, v.z);
	}

	// wzww
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wzww(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.z, v.w, v.w);
	}

	// wwxx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wwxx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.w, v.x, v.x);
	}

	// wwxy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wwxy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.w, v.x, v.y);
	}

	// wwxz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wwxz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.w, v.x, v.z);
	}

	// wwxw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wwxw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.w, v.x, v.w);
	}

	// wwyx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wwyx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.w, v.y, v.x);
	}

	// wwyy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wwyy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.w, v.y, v.y);
	}

	// wwyz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wwyz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.w, v.y, v.z);
	}

	// wwyw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wwyw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.w, v.y, v.w);
	}

	// wwzx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wwzx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.w, v.z, v.x);
	}

	// wwzy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wwzy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.w, v.z, v.y);
	}

	// wwzz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wwzz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.w, v.z, v.z);
	}

	// wwzw
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wwzw(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.w, v.z, v.w);
	}

	// wwwx
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wwwx(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.w, v.w, v.x);
	}

	// wwwy
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wwwy(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.w, v.w, v.y);
	}

	// wwwz
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wwwz(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.w, v.w, v.z);
	}

	// wwww
	template<typename T, precision P>
	GLM_INLINE glm::tvec4<T, P> wwww(const glm::tvec4<T, P> &v) {
		return glm::tvec4<T, P>(v.w, v.w, v.w, v.w);
	}

}
