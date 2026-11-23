/**
 * @file
 */

#pragma once

#include "math/AlignedVec4f.h"
#include "math/AlignedVec4i.h"
#include "math/Vec2f.h"
#include "math/Vec2i.h"
#include "math/Vec3d.h"
#include "math/Vec3f.h"
#include "math/Vec3i.h"
#include "math/Vec3u.h"
#include "math/Vec4f.h"
#include "math/Vec4i.h"

namespace math {

inline constexpr Vec2f::Vec2f(const Vec3f &v) : x(v.x), y(v.y) {
}
inline constexpr Vec2f::Vec2f(const Vec4f &v) : x(v.x), y(v.y) {
}
inline constexpr Vec4f::Vec4f(const Vec2f &xy, float z_, float w_) : x(xy.x), y(xy.y), z(z_), w(w_) {
}
inline constexpr Vec4f::Vec4f(float x_, float y_, const Vec2f &zw) : x(x_), y(y_), z(zw.x), w(zw.y) {
}
inline constexpr Vec4f::Vec4f(const Vec2f &xy, const Vec2f &zw) : x(xy.x), y(xy.y), z(zw.x), w(zw.y) {
}
inline constexpr Vec3f::Vec3f(const Vec3u &v) : x((float)v.x), y((float)v.y), z((float)v.z) {
}
inline constexpr Vec3f::Vec3f(const Vec3d &v) : x((float)v.x), y((float)v.y), z((float)v.z) {
}
inline constexpr Vec3d::Vec3d(const Vec3f &v) : x((double)v.x), y((double)v.y), z((double)v.z) {
}
inline constexpr Vec3d::Vec3d(const Vec3i &v) : x((double)v.x), y((double)v.y), z((double)v.z) {
}
inline constexpr Vec3f::Vec3f(const Vec2f &v, float z_) : x(v.x), y(v.y), z(z_) {
}
inline constexpr Vec3f::Vec3f(const AlignedVec4f &v) : x(v.x), y(v.y), z(v.z) {
}
inline constexpr Vec4f::Vec4f(const AlignedVec4f &v) : x(v.x), y(v.y), z(v.z), w(v.w) {
}
inline constexpr Vec4f::Vec4f(const AlignedVec4i &v) : x((float)v.x), y((float)v.y), z((float)v.z), w((float)v.w) {
}
inline constexpr AlignedVec4f::AlignedVec4f(const Vec4f &v) : x(v.x), y(v.y), z(v.z), w(v.w) {
}
inline constexpr AlignedVec4f::AlignedVec4f(const Vec3f &v, float w_) : x(v.x), y(v.y), z(v.z), w(w_) {
}
inline constexpr AlignedVec4f::AlignedVec4f(const AlignedVec4i &v) : x((float)v.x), y((float)v.y), z((float)v.z), w((float)v.w) {
}
inline constexpr AlignedVec4i::AlignedVec4i(const AlignedVec4f &v) : x((int)v.x), y((int)v.y), z((int)v.z), w((int)v.w) {
}

}
