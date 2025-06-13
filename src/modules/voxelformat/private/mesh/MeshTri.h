/**
 * @file
 */

#pragma once

#include "MeshMaterial.h"
#include "glm/ext/vector_float2.hpp"
#include "math/Tri.h"

namespace voxelformat {

class MeshTri : public math::Tri {
private:
	glm::vec2 _uv[3]{};
public:
	MeshTri() = default;
	inline MeshTri(const glm::vec3 (&v)[3], const glm::vec2 (&uv)[3], const MeshMaterialPtr &_material,
					   const core::RGBA (&c)[3])
		: math::Tri::Tri(v, c) {
		setUVs(uv[0], uv[1], uv[2]);
		this->material = _material;
	}

	MeshMaterialPtr material;

	void setUVs(const glm::vec2 &uv1, const glm::vec2 &uv2, const glm::vec2 &uv3);
	[[nodiscard]] glm::vec2 centerUV() const;

	const glm::vec2 &uv0() const;
	const glm::vec2 &uv1() const;
	const glm::vec2 &uv2() const;

	/**
	 * @return @c false if the given position is not within the triangle area. The value of uv should not be used in
	 * this case.
	 */
	[[nodiscard]] bool calcUVs(const glm::vec3 &pos, glm::vec2 &uv) const;
	[[nodiscard]] core::RGBA colorAt(const glm::vec2 &uv, bool originUpperLeft = false) const;
	[[nodiscard]] core::RGBA extracted() const;
	[[nodiscard]] core::RGBA centerColor() const;
	[[nodiscard]] core::RGBA blendedColor() const;
};

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
	const glm::vec2 miduv[]{glm::mix(in.uv0(), in.uv1(), 0.5f), glm::mix(in.uv1(), in.uv2(), 0.5f), glm::mix(in.uv2(), in.uv0(), 0.5f)};
	const core::RGBA midc[]{core::RGBA::mix(in.color0(), in.color1()), core::RGBA::mix(in.color1(), in.color2()),
							core::RGBA::mix(in.color2(), in.color0())};

	// the subdivided new three triangles
	out[0] = MESHTRI{
		{in.vertex0(), midv[0], midv[2]}, {in.uv0(), miduv[0], miduv[2]}, in.material, {in.color0(), midc[0], midc[2]}};
	out[1] = MESHTRI{
		{in.vertex1(), midv[1], midv[0]}, {in.uv1(), miduv[1], miduv[0]}, in.material, {in.color1(), midc[1], midc[0]}};
	out[2] = MESHTRI{
		{in.vertex2(), midv[2], midv[1]}, {in.uv2(), miduv[2], miduv[1]}, in.material, {in.color2(), midc[2], midc[1]}};
	// keep the middle
	out[3] =
		MESHTRI{{midv[0], midv[1], midv[2]}, {miduv[0], miduv[1], miduv[2]}, in.material, {midc[0], midc[1], midc[2]}};
}

} // namespace voxelformat
