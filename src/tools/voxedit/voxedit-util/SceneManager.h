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
#include "core/Singleton.h"
#include "core/command/ActionButton.h"
#include "math/Axis.h"
#include "MementoHandler.h"
#include "ModifierType.h"
#include "LayerListener.h"
#include "Layer.h"
#include "Modifier.h"
#include "LayerManager.h"
#include <vector>

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

/**
 * @note The data is shared across all viewports
 */
class SceneManager : public core::IComponent, public LayerListener {
private:
	voxelrender::RawVolumeRenderer _volumeRenderer;
	render::GridRenderer _gridRenderer;
	video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;
	MementoHandler _mementoHandler;
	LayerManager _layerMgr;
	Modifier _modifier;
	render::Axis _axis;

	int32_t _referencePointMesh = -1;

	glm::ivec3 _referencePos;

	core::VarPtr _autoSaveSecondsDelay;
	core::VarPtr _ambientColor;
	core::VarPtr _diffuseColor;

	math::Axis _lockedAxis = math::Axis::None;

	struct DirtyRegion {
		voxel::Region region;
		int layer;
	};
	using RegionQueue = std::vector<DirtyRegion>;
	RegionQueue _extractRegions;

	bool _dirty = false;
	// this is basically the same as the dirty state, but we stop
	// auto-saving once we saved a dirty state
	bool _needAutoSave = false;

	bool _renderShadow = true;
	bool _renderAxis = true;
	bool _renderLockAxis = true;

	std::string _lastFilename;
	uint64_t _lastAutoSave = 0u;

	int32_t _planeMeshIndex[3] = {-1, -1, -1};

	int _lastRaytraceX = -1;
	int _lastRaytraceY = -1;

	int _animationSpeed = 0;
	int _currentAnimationLayer = 0;
	uint64_t _nextFrameSwitch = 0;

	int _initialized = 0;
	int _size = 128;
	int _mouseX = 0;
	int _mouseY = 0;

	core::ActionButton _shift;
	uint64_t _lastShift = 0;
	core::ActionButton _move[lengthof(DIRECTIONS)];
	uint64_t _lastMove[lengthof(DIRECTIONS)] { 0 };

	voxel::PickResult _result;
	// existing voxel under the cursor
	voxel::Voxel _hitCursorVoxel;

	voxel::RawVolume* modelVolume();
	bool setNewVolume(int idx, voxel::RawVolume* volume, bool deleteMesh = true);
	bool setNewVolumes(const voxel::VoxelVolumes& volumes);
	void autosave();
	void setReferencePosition(const glm::ivec3& pos);

	void animate(uint64_t time);
	/**
	 * @brief Move the cursor relative by the given steps in each direction
	 */
	void moveCursor(int x, int y, int z);

	void crop();
	void resize(const glm::ivec3& size);
	int size() const;

	/**
	 * @brief Merge two layers and extend the smaller one
	 */
	bool merge(int layerId1, int layerId2);

	void undo();
	void redo();

	/**
	 * @brief Convert a given point cloud to voxels
	 * @param[in] vertices 3 component vertex data.
	 * @param[in] vertexColors 3 component color data
	 * @param[in] amount The amount of vertices in the buffers
	 * @note The given @c vertices coordinates must be aligned to the region of the volume already
	 * @note The color is expected to be in the range [0.0f,1.0f]
	 */
	void pointCloud(const glm::vec3* vertices, const glm::vec3 *vertexColors, size_t amount);

	void rotate(int layerId, const glm::ivec3& angle, bool increaseSize = false, bool rotateAroundReferencePosition = false);
	void rotate(int angleX, int angleY, int angleZ, bool increaseSize = false, bool rotateAroundReferencePosition = false);

	void move(int layerId, const glm::ivec3& m);
	void move(int x, int y, int z);

	void shift(int layerId, const glm::ivec3& m);
	void shift(int x, int y, int z);
	void shiftAlongAxis();

	bool extractVolume();
	void updateLockedPlane(math::Axis axis);
public:
	SceneManager();
	~SceneManager();

	void resetLastTrace();
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

	const glm::ivec3& referencePosition() const;

	void construct() override;
	bool init() override;
	void update(uint64_t time);
	void shutdown() override;

	voxelrender::RawVolumeRenderer& renderer();
	const voxelrender::RawVolumeRenderer& renderer() const;

	void modified(int layerId, const voxel::Region& modifiedRegion, bool markUndo = true);
	voxel::RawVolume* volume(int idx);

	/**
	 * @brief Voxelizes the given mesh
	 * @param[in] mesh The video::Mesh to voxelize
	 * @return @c true on success, @c false on failure
	 */
	bool voxelizeModel(const video::MeshPtr& mesh);
	/**
	 * @brief Import a heightmap in the current layer of the scene
	 * @param[in] file The image file to import as heightmap.
	 * @note The first component is used as height value. In most cases the R channel.
	 */
	bool importHeightmap(const std::string& file);
	/**
	 * @brief Import an image as a plane in a new layer of the scene.
	 * @note There is a total max dimension of the texture that is supported.
	 * @param[in] file The image file path to load
	 */
	bool importAsPlane(const std::string& file);
	/**
	 * @brief Import a new palette from the given image file
	 * @note The amount of colors in the image may not exceed 256
	 * @param[in] file The image file path
	 */
	bool importPalette(const std::string& file);
	/**
	 * @param[in] paletteName The name of the palette
	 * @note The name is extended to a filename like this @c palette-<paletteName>.[lua.png]
	 */
	bool loadPalette(const std::string& paletteName);
	/**
	 * @brief Exports the volume data to a supported mesh format identified by the file extension
	 */
	bool exportModel(const std::string& file);
	/**
	 * @brief Save the volume data to the given file
	 * @param[in] file The file to store the volume data in. The file extension defines the volume format.
	 * @param[in] autosave @c true if this is an auto save action, @c false otherwise. This has e.g. an
	 * influence on the dirty state handling of the scene.
	 */
	bool save(const std::string& file, bool autosave = false);
	/**
	 * @brief Loads a volume from the given file
	 * @param[in] file The file to load. The volume format is determined by the file extension.
	 */
	bool load(const std::string& file);
	/**
	 * @brief Import an existing model
	 * @note Placed relative to the reference position in the current scene
	 */
	bool prefab(const std::string& file);

	bool newScene(bool force, const std::string& name, const voxel::Region& region);

	bool dirty() const;
	bool empty() const;

	static const uint8_t RenderScene = 1u << 0u;
	static const uint8_t RenderUI    = 1u << 1u;
	static const uint8_t RenderAll   = RenderScene | RenderUI;

	/**
	 * @brief Performs the rendering for each @c Viewport instance
	 */
	void render(const video::Camera& camera, uint8_t renderMask = RenderAll);

	render::GridRenderer& gridRenderer();
	bool setGridResolution(int resolution);

	void noise(int octaves, float persistence, float lacunarity, float gain, voxel::noisegen::NoiseType type);
	void createTree(voxel::TreeContext ctx);
	void createBuilding(voxel::BuildingType type, const voxel::BuildingContext& ctx);
	void createPlant(voxel::PlantType type);
	void createCloud();
	void createCactus();

	void setMousePos(int x, int y);

	bool trace(const video::Camera& camera, bool force = false);

	math::Axis lockedAxis() const;
	void setLockedAxis(math::Axis axis, bool unlock);
	void setRenderAxis(bool renderAxis);
	void setRenderLockAxis(bool renderLockAxis);
	void setRenderShadow(bool shadow);

	const LayerManager& layerMgr() const;
	LayerManager& layerMgr();
	const Modifier& modifier() const;
	Modifier& modifier();
	const MementoHandler& mementoHandler() const;
	MementoHandler& mementoHandler();

	void onLayerChanged(int layerId) override;
	void onLayerDuplicate(int layerId) override;
	void onLayerSwapped(int layerId1, int layerId2) override;
	void onLayerHide(int layerId) override;
	void onLayerShow(int layerId) override;
	void onActiveLayerChanged(int old, int active) override;
	void onLayerAdded(int layerId, const Layer& layer, voxel::RawVolume* volume, const voxel::Region& region) override;
	void onLayerDeleted(int layerId, const Layer& layer) override;
};

inline voxelrender::RawVolumeRenderer& SceneManager::renderer() {
	return _volumeRenderer;
}

inline const voxelrender::RawVolumeRenderer& SceneManager::renderer() const {
	return _volumeRenderer;
}

inline math::Axis SceneManager::lockedAxis() const {
	return _lockedAxis;
}

inline const MementoHandler& SceneManager::mementoHandler() const {
	return _mementoHandler;
}

inline MementoHandler& SceneManager::mementoHandler() {
	return _mementoHandler;
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
	return _layerMgr.layers().empty();
}

inline const voxel::Voxel& SceneManager::hitCursorVoxel() const {
	return _hitCursorVoxel;
}

inline const glm::ivec3& SceneManager::cursorPosition() const {
	return _modifier.cursorPosition();
}

inline const glm::ivec3& SceneManager::referencePosition() const {
	return _referencePos;
}

inline const LayerManager& SceneManager::layerMgr() const {
	return _layerMgr;
}

inline LayerManager& SceneManager::layerMgr() {
	return _layerMgr;
}

inline const Modifier& SceneManager::modifier() const {
	return _modifier;
}

inline Modifier& SceneManager::modifier() {
	return _modifier;
}

inline SceneManager& sceneMgr() {
	return core::Singleton<SceneManager>::getInstance();
}

}
