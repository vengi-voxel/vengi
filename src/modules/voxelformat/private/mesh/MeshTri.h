/**
 * @file
 */

#pragma once

#include "MeshMaterial.h"
#include "math/Tri.h"
#include <glm/vec2.hpp>

namespace voxelformat {

class MeshTri : public math::Tri {
private:
	glm::vec2 _uv[3]{};

public:
	MeshTri() = default;
	inline MeshTri(const glm::vec3 (&v)[3], const glm::vec2 (&uv)[3], MeshMaterialIndex _materialIdx,
				   const color::RGBA (&c)[3])
		: math::Tri::Tri(v, c) {
		setUVs(uv[0], uv[1], uv[2]);
		this->materialIdx = _materialIdx;
	}

	MeshMaterialIndex materialIdx = -1;

	void setUVs(const glm::vec2 &uv1, const glm::vec2 &uv2, const glm::vec2 &uv3);
	[[nodiscard]] glm::vec2 centerUV() const;

	const glm::vec2 &uv0() const;
	const glm::vec2 &uv1() const;
	const glm::vec2 &uv2() const;

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
	[[nodiscard]] bool calcUVs(const glm::vec3 &pos, glm::vec2 &uv) const;
};
// static_assert(sizeof(MeshTri) == 76, "Unexpected size for MeshTri - try to keep this small");

inline const glm::vec2 &MeshTri::uv0() const {
	return _uv[0];
}

inline const glm::vec2 &MeshTri::uv1() const {
	return _uv[1];
}

inline const glm::vec2 &MeshTri::uv2() const {
	return _uv[2];
}

// Sierpinski gasket with keeping the middle
template<class MESHTRI>
inline void subdivide(const MESHTRI &in, MESHTRI out[4]) {
	const glm::vec3 midv[]{glm::mix(in.vertex0(), in.vertex1(), 0.5f), glm::mix(in.vertex1(), in.vertex2(), 0.5f),
						   glm::mix(in.vertex2(), in.vertex0(), 0.5f)};
	const glm::vec2 miduv[]{glm::mix(in.uv0(), in.uv1(), 0.5f), glm::mix(in.uv1(), in.uv2(), 0.5f),
							glm::mix(in.uv2(), in.uv0(), 0.5f)};
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

inline color::RGBA colorAt(const MeshTri &tri, const MeshMaterialArray &meshMaterialArray, const glm::vec2 &uv, bool originUpperLeft = false) {
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

	const auto mixColors = [](const color::RGBA &a, const color::RGBA &b, const color::RGBA &c) {
		return color::RGBA::mix(color::RGBA::mix(a, b), c);
	};

	if (material) {
		return mixColors(material->apply(tri.color0()), material->apply(tri.color1()),
						 material->apply(tri.color2()));
	}

	return mixColors(tri.color0(), tri.color1(), tri.color2());
}

} // namespace voxelformat
