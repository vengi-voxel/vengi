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
#include "voxelformat/VoxFileFormat.h"
#include "video/ShapeBuilder.h"
#include "video/Mesh.h"
#include "render/ShapeRenderer.h"
#include "render/GridRenderer.h"
#include "render/Axis.h"
#include "core/Var.h"
#include "core/command/ActionButton.h"
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

/**
 * @brief Move directions for the cursor
 */
static constexpr struct Direction {
	const char *postfix;
	int x;
	int y;
	int z;
} DIRECTIONS[] = {
	{"left",      1,  0,  0},
	{"right",    -1,  0,  0},
	{"up",        0,  1,  0},
	{"down",      0, -1,  0},
	{"forward",   0,  0,  1},
	{"backward",  0,  0, -1}
};

struct Layer {
	std::string name;
	bool visible = true;
	bool valid = false;

	void reset() {
		name.clear();
		visible = true;
		valid = false;
	}
};
using Layers = std::array<Layer, voxelrender::RawVolumeRenderer::MAX_VOLUMES>;

class SceneListener {
public:
	virtual ~SceneListener() {}

	virtual void onLayerHide(int layerId) {}
	virtual void onLayerShow(int layerId) {}
	virtual void onActiveLayerChanged(int old, int active) {}
	virtual void onLayerAdded(int layerId, const Layer& layer) {}
	virtual void onLayerDeleted(int layerId) {}
};

/**
 * @note The data is shared across all viewports
 */
class SceneManager : public core::IComponent {
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

	struct DirtyRegion {
		voxel::Region region;
		int layer;
	};
	using RegionQueue = std::vector<DirtyRegion>;
	RegionQueue _extractRegions;

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

	int _animationSpeed = 0;
	int _currentAnimationLayer = 0;
	uint64_t _nextFrameSwitch = 0;

	int _initialized = 0;
	int _size = 128;
	int _mouseX = 0;
	int _mouseY = 0;

	int _activeLayer = 0;

	core::ActionButton _move[lengthof(DIRECTIONS)];
	uint64_t _lastMove[lengthof(DIRECTIONS)] { 0 };

	voxel::PickResult _result;
	voxel::Voxel _cursorVoxel;
	// existing voxel under the cursor
	voxel::Voxel _hitCursorVoxel;

	Layers _layers;
	SceneListener *_listener = nullptr;

	ModifierType _modifierType = ModifierType::Place;
	bool modifierTypeRequiresExistingVoxel() const;

	int getIndexForAxis(math::Axis axis) const;
	int getIndexForMirrorAxis(math::Axis axis) const;
	void updateShapeBuilderForPlane(bool mirror, const glm::ivec3& pos, math::Axis axis, const glm::vec4& color);
	void modified(int layerId, const voxel::Region& modifiedRegion, bool markUndo = true);

	voxel::RawVolume* volume(int idx);
	voxel::RawVolume* modelVolume();
	bool setNewVolume(int idx, voxel::RawVolume* volume);
	bool setNewVolumes(const voxel::VoxelVolumes& volumes);
	void resetLastTrace();
	bool getMirrorAABB(glm::ivec3& mins, glm::ivec3& maxs) const;

	void executeModifier();
	void animate(uint64_t time);
public:
	SceneManager();
	~SceneManager();

	voxel::Region region() const;

	const voxel::Voxel& hitCursorVoxel() const;

	const glm::ivec3& cursorPosition() const;
	/**
	 * @brief Sets the cursor to the given position in the volume
	 * @note The locked axes are taken into account here and the given position might not be
	 * the final position of the cursor
	 * @param[in] force If @c true it will ignore the locked axes and still set the position
	 */
	void setCursorPosition(glm::ivec3 pos, bool force = false);

	/**
	 * @brief Move the cursor relative by the given steps in each direction
	 */
	void moveCursor(int x, int y, int z);

	const glm::ivec3& referencePosition() const;
	void setReferencePosition(const glm::ivec3& pos);

	void construct() override;
	bool init() override;
	void update(uint64_t time);
	void shutdown() override;
	void autosave();

	glm::ivec3 aabbPosition() const;
	bool aabbMode() const;
	glm::ivec3 aabbDim() const;

	bool aabbStart();
	bool aabbEnd(bool trace = true);

	void crop();
	void resize(const glm::ivec3& size);

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
	/**
	 * @brief Import an existing model
	 * @note Placed relative to the reference position in the current scene
	 */
	bool prefab(const std::string& file);

	bool newScene(bool force);

	bool dirty() const;
	bool empty() const;
	int size() const;

	void rotate(int angleX, int angleY, int angleZ);
	void move(int x, int y, int z);

	/**
	 * @brief Performs the rendering for each @c Viewport instance
	 */
	void render(const video::Camera& camera);

	void setCursorVoxel(const voxel::Voxel& voxel);
	const voxel::Voxel& cursorVoxel() const;

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
	void setModifierType(ModifierType type, bool trace = true);
	bool addModifierType(ModifierType type, bool trace = true);

	void undo();
	void redo();

	void registerListener(SceneListener* listener);
	void unregisterListener(SceneListener* listener);

	bool findNewActiveLayer();
	int activeLayer() const;
	bool setActiveLayer(int layerId);
	const Layers& layers() const;
	int validLayers() const;
	int validLayerId(int index) const;
	void hideLayer(int layerId, bool hide);
	bool deleteLayer(int layerId, bool force = false);
	int addLayer(const char *name, bool visible = true, voxel::RawVolume* volume = nullptr);

	const MementoHandler& mementoHandler() const;
};

inline math::Axis SceneManager::lockedAxis() const {
	return _lockedAxis;
}

inline const MementoHandler& SceneManager::mementoHandler() const {
	return _mementoHandler;
}

inline int SceneManager::activeLayer() const {
	return _activeLayer;
}

inline int SceneManager::gridResolution() const {
	return _gridRenderer.gridResolution();
}

inline render::GridRenderer& SceneManager::gridRenderer() {
	return _gridRenderer;
}

inline bool SceneManager::dirty() const {
	return _dirty;
}

inline int SceneManager::size() const {
	return _size;
}

inline bool SceneManager::empty() const {
	return _layers.empty();
}

inline const voxel::Voxel& SceneManager::hitCursorVoxel() const {
	return _hitCursorVoxel;
}

inline const glm::ivec3& SceneManager::cursorPosition() const {
	return _cursorPos;
}

inline const glm::ivec3& SceneManager::referencePosition() const {
	return _referencePos;
}

inline const Layers& SceneManager::layers() const {
	return _layers;
}

inline const voxel::Voxel& SceneManager::cursorVoxel() const {
	return _cursorVoxel;
}

static SceneManager& sceneMgr();

}
