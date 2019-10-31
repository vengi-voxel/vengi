/**
 * @file
 */

#pragma once

#include "video/ShapeBuilder.h"
#include "render/ShapeRenderer.h"
#include "core/GLM.h"
#include "core/IComponent.h"
#include "video/Camera.h"
#include "voxel/Voxel.h"
#include "math/Axis.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "ModifierType.h"
#include "ModifierButton.h"
#include "math/AABB.h"
#include "Selection.h"

namespace voxedit {

class Modifier : public core::IComponent {
private:
	Selection _selection;
	bool _selectionValid = false;
	glm::ivec3 _aabbFirstPos;
	bool _aabbMode = false;
	ModifierType _modifierType = ModifierType::Place;
	video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;
	int32_t _aabbMeshIndex = -1;
	int32_t _selectionIndex = -1;
	int _gridResolution = 1;
	int32_t _mirrorMeshIndex = -1;
	math::Axis _mirrorAxis = math::Axis::None;
	glm::ivec3 _mirrorPos {0};
	glm::ivec3 _cursorPosition {0};
	voxel::FaceNames _face = voxel::FaceNames::NoOfFaces;
	voxel::Voxel _cursorVoxel;
	int32_t _voxelCursorMesh = -1;
	ModifierButton _actionExecuteButton;
	ModifierButton _deleteExecuteButton;

	bool getMirrorAABB(glm::ivec3& mins, glm::ivec3& maxs) const;
	glm::ivec3 aabbPosition() const;
	void updateMirrorPlane();
	void renderAABBMode(const video::Camera& camera);
	void updateSelectionBuffers();
	bool select(const glm::ivec3& mins, const glm::ivec3& maxs, voxel::RawVolume* volume, std::function<void(const voxel::Region& region, ModifierType type)> callback);
public:
	Modifier();

	void construct() override;
	bool init() override;
	void shutdown() override;

	void translate(const glm::ivec3& v);

	const Selection& selection() const;

	math::Axis mirrorAxis() const;
	void setMirrorAxis(math::Axis axis, const glm::ivec3& mirrorPos);

	ModifierType modifierType() const;
	void setModifierType(ModifierType type);

	const voxel::Voxel& cursorVoxel() const;
	void setCursorVoxel(const voxel::Voxel& voxel);

	bool aabbMode() const;
	glm::ivec3 aabbDim() const;

	/**
	 * @brief Pick the start position of the modifier execution bounding box
	 */
	bool aabbStart();
	/**
	 * @brief End the current ModifierType execution and modify tha given volume according to the type.
	 * @param[out,in] volume The volume to modify
	 * @param callback Called for every region that was modified for the current active modifier.
	 */
	bool aabbAction(voxel::RawVolume* volume, std::function<void(const voxel::Region& region, ModifierType type)> callback);
	void aabbStop();

	bool modifierTypeRequiresExistingVoxel() const;

	void setCursorPosition(const glm::ivec3& pos, voxel::FaceNames face);
	const glm::ivec3& cursorPosition() const;
	voxel::FaceNames cursorFace() const;

	/**
	 * @note Mirrored REMOVE ME
	 */
	void setGridResolution(int resolution);

	void render(const video::Camera& camera);
};

inline voxel::FaceNames Modifier::cursorFace() const {
	return _face;
}

inline const voxel::Voxel& Modifier::cursorVoxel() const {
	return _cursorVoxel;
}

inline const glm::ivec3& Modifier::cursorPosition() const {
	return _cursorPosition;
}

inline const Selection& Modifier::selection() const {
	return _selection;
}

}
