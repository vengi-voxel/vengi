/**
 * @file
 */

#pragma once

#include "MeshMaterial.h"
#include "math/Tri.h"
#include "math/Vec3f.h"
#include "math/Vec2f.h"

namespace voxelformat {

class MeshTri : public math::Tri {
private:
	math::Vec2f _uv[3]{};

public:
	MeshTri() = default;
	inline MeshTri(const math::Vec3f (&v)[3], const math::Vec2f (&uv)[3], MeshMaterialIndex _materialIdx,
				   const color::RGBA (&c)[3])
		: math::Tri::Tri(v, c) {
		setUVs(uv[0], uv[1], uv[2]);
		this->materialIdx = _materialIdx;
	}

	MeshMaterialIndex materialIdx = -1;

	void setUVs(const math::Vec2f &uv1, const math::Vec2f &uv2, const math::Vec2f &uv3);
	[[nodiscard]] math::Vec2f centerUV() const;

	const math::Vec2f &uv0() const;
	const math::Vec2f &uv1() const;
	const math::Vec2f &uv2() const;

	float maxSideLength() const;
	/**
	 * Estimate reserve size: each subdivision splits a triangle into 4.
	 * We need d subdivisions where longest_side * (0.5^d) <= 1 => d = ceil(log2(longest_side)).
	 * The number of triangles produced per original triangle is 4^d.
	 */
	int subdivideTriCount(size_t maxPerTriangle) const;

	/**
	 * @return @c false if the given position is not within the triangle area. The value of uv should not be used in
	 * this case.
	 */
	[[nodiscard]] bool calcUVs(const math::Vec3f &pos, math::Vec2f &uv) const;
};
// static_assert(sizeof(MeshTri) == 76, "Unexpected size for MeshTri - try to keep this small");

inline const math::Vec2f &MeshTri::uv0() const {
	return _uv[0];
}

inline const math::Vec2f &MeshTri::uv1() const {
	return _uv[1];
}

inline const math::Vec2f &MeshTri::uv2() const {
	return _uv[2];
}

// Sierpinski gasket with keeping the middle
template<class MESHTRI>
inline void subdivide(const MESHTRI &in, MESHTRI out[4]) {
	const math::Vec3f midv[]{math::mix(in.vertex0(), in.vertex1(), 0.5f), math::mix(in.vertex1(), in.vertex2(), 0.5f),
						   math::mix(in.vertex2(), in.vertex0(), 0.5f)};
	const math::Vec2f miduv[]{math::mix(in.uv0(), in.uv1(), 0.5f), math::mix(in.uv1(), in.uv2(), 0.5f),
							math::mix(in.uv2(), in.uv0(), 0.5f)};
	const color::RGBA midc[]{color::RGBA::mix(in.color0(), in.color1()), color::RGBA::mix(in.color1(), in.color2()),
							color::RGBA::mix(in.color2(), in.color0())};

	// the subdivided new three triangles
	out[0] = MESHTRI{
		{in.vertex0(), midv[0], midv[2]}, {in.uv0(), miduv[0], miduv[2]}, in.materialIdx, {in.color0(), midc[0], midc[2]}};
	out[1] = MESHTRI{
		{in.vertex1(), midv[1], midv[0]}, {in.uv1(), miduv[1], miduv[0]}, in.materialIdx, {in.color1(), midc[1], midc[0]}};
	out[2] = MESHTRI{
		{in.vertex2(), midv[2], midv[1]}, {in.uv2(), miduv[2], miduv[1]}, in.materialIdx, {in.color2(), midc[2], midc[1]}};
	// keep the middle
	out[3] =
		MESHTRI{{midv[0], midv[1], midv[2]}, {miduv[0], miduv[1], miduv[2]}, in.materialIdx, {midc[0], midc[1], midc[2]}};
}

inline color::RGBA colorAt(const MeshTri &tri, const MeshMaterialArray &meshMaterialArray, const math::Vec2f &uv, bool originUpperLeft = false, const math::Vec3f *weights = nullptr) {
	MeshMaterial* material;
	if (tri.materialIdx >= 0 && tri.materialIdx < (int)meshMaterialArray.size()) {
		material = meshMaterialArray[tri.materialIdx].get();
	} else {
		material = nullptr;
	}
	color::RGBA rgba;
	if (material && material->colorAt(rgba, uv, originUpperLeft)) {
		return rgba;
	}

	const auto mixColors = [weights](const color::RGBA &a, const color::RGBA &b, const color::RGBA &c) {
		if (weights) {
			return color::RGBA(
				(uint8_t)math::round(a.r * weights->x + b.r * weights->y + c.r * weights->z),
				(uint8_t)math::round(a.g * weights->x + b.g * weights->y + c.g * weights->z),
				(uint8_t)math::round(a.b * weights->x + b.b * weights->y + c.b * weights->z),
				(uint8_t)math::round(a.a * weights->x + b.a * weights->y + c.a * weights->z));
		}
		return color::RGBA::mix(color::RGBA::mix(a, b), c);
	};

	if (material) {
		return mixColors(material->apply(tri.color0()), material->apply(tri.color1()),
						 material->apply(tri.color2()));
	}

	return mixColors(tri.color0(), tri.color1(), tri.color2());
}

} // namespace voxelformat
