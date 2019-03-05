/**
 * @file
 */

#pragma once

#include "voxel/polyvox/Picking.h"
#include "voxel/polyvox/RawVolume.h"
#include "voxel/generator/PlantType.h"
#include "voxel/TreeContext.h"
#include "voxel/generator/BuildingGeneratorContext.h"
#include "voxel/generator/NoiseGenerator.h"
#include "voxelrender/RawVolumeRenderer.h"
#include "video/ShapeBuilder.h"
#include "render/ShapeRenderer.h"
#include "render/GridRenderer.h"
#include "render/Axis.h"
#include "video/MeshPool.h"
#include "math/Axis.h"
#include "voxedit-util/MementoHandler.h"
#include "voxedit-util/ModifierType.h"
#include <vector>

namespace voxel {
namespace tree {
class Tree;
}
}

namespace voxedit {

static constexpr int ModelVolumeIndex = 0;

/**
 * @note The data is shared across all viewports
 */
class ViewportSingleton : public core::IComponent {
private:
	voxelrender::RawVolumeRenderer _volumeRenderer;
	render::GridRenderer _gridRenderer;
	video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;
	MementoHandler _mementoHandler;
	render::Axis _axis;

	int32_t _referencePointMesh = -1;
	int32_t _voxelCursorMesh = -1;

	glm::ivec3 _cursorPos;
	glm::ivec3 _referencePos;
	glm::ivec3 _mirrorPos;

	glm::ivec3 _aabbFirstPos;
	bool _aabbMode = false;
	core::VarPtr _autoSaveSecondsDelay;

	math::Axis _lockedAxis = math::Axis::None;
	math::Axis _mirrorAxis = math::Axis::None;

	using RegionQueue = std::vector<voxel::Region>;
	RegionQueue _extractRegions;

	bool _empty = true;
	bool _dirty = false;
	bool _needAutoSave = false;
	bool _extract = false;

	bool _renderShadow = true;
	bool _renderAxis = true;
	bool _renderLockAxis = true;

	std::string _lastFilename;
	uint64_t _lastAutoSave = 0u;

	int32_t _planeMeshIndex[3] = {-1, -1, -1};
	int32_t _mirrorMeshIndex = -1;
	int32_t _aabbMeshIndex = -1;

	int _lastRaytraceX = -1;
	int _lastRaytraceY = -1;

	int _initialized = 0;
	int _size = 128;
	int _mouseX = 0;
	int _mouseY = 0;

	voxel::PickResult _result;
	voxel::Voxel _cursorVoxel;

	ModifierType _modifierType = ModifierType::Place;
	bool modifierTypeRequiresExistingVoxel() const;

	int getIndexForAxis(math::Axis axis) const;
	int getIndexForMirrorAxis(math::Axis axis) const;
	void updateShapeBuilderForPlane(bool mirror, const glm::ivec3& pos, math::Axis axis, const glm::vec4& color);
	void modified(const voxel::Region& modifiedRegion, bool markUndo = true);
	ViewportSingleton();

	voxel::RawVolume* modelVolume();
	void setNewVolume(voxel::RawVolume* volume);
	void resetLastTrace();
	bool getMirrorAABB(glm::ivec3& mins, glm::ivec3& maxs) const;
public:
	~ViewportSingleton();

	static inline voxedit::ViewportSingleton& getInstance() {
		static voxedit::ViewportSingleton singleton;
		return singleton;
	}

	voxel::Region region() const;

	const glm::ivec3& cursorPosition() const;
	void setCursorPosition(glm::ivec3 pos, bool force = false);

	const glm::ivec3& referencePosition() const;
	void setReferencePosition(const glm::ivec3& pos);

	bool init() override;
	void update();
	void shutdown() override;
	void autosave();

	bool aabbMode() const;
	glm::ivec3 aabbDim() const;

	bool aabbStart();
	bool aabbEnd();

	void crop();
	void extend(const glm::ivec3& size);
	void scaleHalf();

	/**
	 * @brief Convert a given point cloud to voxels
	 * @param[in] vertices 3 component vertex data.
	 * @param[in] vertexColors 3 component color data
	 * @param[in] amount The amount of vertices in the buffers
	 * @note The given @c vertices coordinates must be aligned to the region of the volume already
	 * @note The color is expected to be in the range [0.0f,1.0f]
	 */
	void pointCloud(const glm::vec3* vertices, const glm::vec3 *vertexColors, size_t amount);
	bool voxelizeModel(const video::MeshPtr& mesh);
	bool importHeightmap(const std::string& file);
	bool exportModel(const std::string& file);
	bool save(const std::string& file, bool autosave = false);
	bool load(const std::string& file);
	bool prefab(const std::string& file);

	bool newVolume(bool force);

	bool dirty() const;
	bool empty() const;
	int size() const;

	void rotate(int angleX, int angleY, int angleZ);
	void move(int x, int y, int z);

	void render(const video::Camera& camera);

	void setCursorVoxel(const voxel::Voxel& voxel);

	render::GridRenderer& gridRenderer();
	int gridResolution() const;
	bool setGridResolution(int resolution);

	void noise(int octaves, float persistence, float lacunarity, float gain, voxel::noisegen::NoiseType type);
	void createTree(voxel::TreeContext ctx);
	void createBuilding(voxel::BuildingType type, const voxel::BuildingContext& ctx);
	void createPlant(voxel::PlantType type);
	void createCloud();
	void createCactus();

	bool extractVolume();

	void setMousePos(int x, int y);

	bool trace(const video::Camera& camera, bool force = false);

	math::Axis lockedAxis() const;
	void setLockedAxis(math::Axis axis, bool unlock);
	void updateLockedPlane(math::Axis axis);

	math::Axis mirrorAxis() const;
	void setMirrorAxis(math::Axis axis, const glm::ivec3& mirrorPos);
	void updateMirrorPlane();

	bool renderAxis() const;
	void setRenderAxis(bool renderAxis);

	bool renderLockAxis() const;
	void setRenderLockAxis(bool renderLockAxis);

	bool renderShadow() const;
	void setRenderShadow(bool shadow);

	ModifierType modifierType() const;
	void setModifierType(ModifierType type);
	bool addModifierType(ModifierType type);

	void undo();
	void redo();

	const MementoHandler& mementoHandler() const;
};

inline math::Axis ViewportSingleton::lockedAxis() const {
	return _lockedAxis;
}

inline const MementoHandler& ViewportSingleton::mementoHandler() const {
	return _mementoHandler;
}

inline voxel::RawVolume* ViewportSingleton::modelVolume() {
	return _volumeRenderer.volume(ModelVolumeIndex);
}

inline int ViewportSingleton::gridResolution() const {
	return _gridRenderer.gridResolution();
}

inline render::GridRenderer& ViewportSingleton::gridRenderer() {
	return _gridRenderer;
}

inline bool ViewportSingleton::dirty() const {
	return _dirty;
}

inline int ViewportSingleton::size() const {
	return _size;
}

inline bool ViewportSingleton::empty() const {
	return _empty;
}

inline const glm::ivec3& ViewportSingleton::cursorPosition() const {
	return _cursorPos;
}

inline const glm::ivec3& ViewportSingleton::referencePosition() const {
	return _referencePos;
}

}
