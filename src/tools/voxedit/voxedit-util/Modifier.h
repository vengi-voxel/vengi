/**
 * @file
 */

#pragma once

#include "video/ShapeBuilder.h"
#include "render/ShapeRenderer.h"
#include "core/GLM.h"
#include "core/IComponent.h"
#include "video/Camera.h"
#include "voxel/polyvox/Voxel.h"

#include "ModifierType.h"

namespace voxedit {

class Modifier : public core::IComponent {
private:
	glm::ivec3 _aabbFirstPos;
	bool _aabbMode = false;
	ModifierType _modifierType = ModifierType::Place;
	video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;
	int32_t _aabbMeshIndex = -1;
	int _gridResolution = 1;
	int32_t _mirrorMeshIndex = -1;
	math::Axis _mirrorAxis = math::Axis::None;
	glm::ivec3 _mirrorPos;
	voxel::Voxel _cursorVoxel;
	int32_t _voxelCursorMesh = -1;

	bool getMirrorAABB(glm::ivec3& mins, glm::ivec3& maxs) const;

	void executeModifier();
public:
	void construct() override;
	bool init() override;
	void shutdown() override;

	math::Axis mirrorAxis() const;
	void setMirrorAxis(math::Axis axis, const glm::ivec3& mirrorPos);
	void updateMirrorPlane();

	ModifierType modifierType() const;
	void setModifierType(ModifierType type, bool trace = true);
	bool addModifierType(ModifierType type, bool trace = true);

	void setCursorVoxel(const voxel::Voxel& voxel);
	const voxel::Voxel& cursorVoxel() const;

	void setGridResolution(int resolution);

	glm::ivec3 aabbPosition() const;
	bool aabbMode() const;
	glm::ivec3 aabbDim() const;

	bool aabbStart();
	bool aabbEnd(bool trace = true);

	bool modifierTypeRequiresExistingVoxel() const;

	void render(const video::Camera& camera);
};

inline const voxel::Voxel& Modifier::cursorVoxel() const {
	return _cursorVoxel;
}

}
