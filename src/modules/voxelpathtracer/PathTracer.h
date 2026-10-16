/**
 * @file
 */

#pragma once

#include "GpuSceneData.h"
#include "PathTracerScene.h"
#include "PathTracerState.h"
#include "VoxelpathtracerBloomData.h"
#include "VoxelpathtracerData.h"
#include "VoxelpathtracerDisplayData.h"
#include "core/NonCopyable.h"
#include "core/SharedPtr.h"
#include "core/collection/Buffer.h"
#include "video/Buffer.h"
#include "video/Camera.h"
#include "video/FrameBuffer.h"
#include "video/Texture.h"
#include <glm/vec3.hpp>

namespace image {
class Image;
typedef core::SharedPtr<Image> ImagePtr;
} // namespace image

namespace palette {
class Palette;
} // namespace palette

namespace scenegraph {
class SceneGraph;
} // namespace scenegraph

namespace voxelpathtracer {

class PathTracer : public core::NonCopyable {
private:
	PathTracerState _state;
	bool _initialized = false;
	video::Buffer _vbo;
	int32_t _bufferIndex = -1;
	int32_t _texIndex = -1;
	video::TexturePtr _bricksTex;
	video::TexturePtr _indirectionTex;
	video::TexturePtr _paletteTex;
	video::TexturePtr _materialTex;
	video::TexturePtr _emitTex;
	int _lightCount = 0;
	float _emitterLuminance = 0.0f;
	int _bricksPerRow = 1;
	int _bricksPerCol = 1;
	video::FrameBuffer _accum[2];
	video::FrameBuffer _displayFbo;
	video::FrameBuffer _bloomFbo;
	shader::VoxelpathtracerBloomData _bloomUbo;
	shader::VoxelpathtracerData _paramsUbo;
	alignas(16) shader::VoxelpathtracerData::ParamsData _paramsUniforms;
	shader::VoxelpathtracerDisplayData _displayUbo;
	alignas(16) shader::VoxelpathtracerDisplayData::DisplayData _displayUniforms;
	int _writeAccum = 0;
	int _targetResolution = 0;
	glm::ivec3 _volumeMin{0};
	glm::ivec3 _volumeSize{0};
	bool _hasVolume = false;
	bool _displayDirty = true;
	bool _bloomDirty = true;
	float _floorY = 0.0f;
	video::Camera _traceCamera;

	bool initGpu();
	void shutdownGpu();
	bool createScene(const scenegraph::SceneGraph &sceneGraph, const video::Camera *camera);
	void setupTraceCamera(const scenegraph::SceneGraph &sceneGraph, const video::Camera *camera);
	bool createEmptyGpuScene();
	bool uploadSparseVolume(const SparseGpuVolume &volume, size_t materialCount);
	bool uploadMaterialAtlas(const GpuMaterialAtlas &atlas, const SparseGpuVolume &volume);
	bool uploadPreparedScene(PreparedGpuScene &prepared);
	bool ensureTargets(int resolution);
	int outputResolution() const;
	void uploadEmitLights(const GpuEmitLight *lights, int count);
	void drawFullscreen(video::FrameBuffer &target);
	void tracePass(int samples);
	void displayPass();
	bool bloomPass(float exposure);
	bool cameraPoseChanged(const video::Camera &camera) const;

public:
	PathTracer() = default;
	~PathTracer();
	PathTracerState &state() {
		return _state;
	}
	const PathTracerState &state() const {
		return _state;
	}

	bool init();
	void shutdown();

	bool start(const scenegraph::SceneGraph &sceneGraph, const video::Camera *camera = nullptr);
	/**
	 * @brief Upload a CPU-prepared scene and start sampling.
	 * Call on the GL thread after @c prepareGpuScene() finishes on a worker.
	 */
	bool startPrepared(PreparedGpuScene &&prepared, const scenegraph::SceneGraph &sceneGraph,
					   const video::Camera *camera = nullptr);
	bool restart(const scenegraph::SceneGraph &sceneGraph, const video::Camera *camera = nullptr);
	bool stop();
	/** @brief Continue sampling after @c stop() without rebuilding the GPU scene. */
	bool resume();
	bool started() const;
	bool hasVolume() const;
	void applyCamera(const video::Camera *camera);
	void resetAccumulation();

	/** @brief @c Limit::Max3DTextureSize clamped for pathtracer brick atlases. Call on the renderer thread. */
	static int maxTextureAxis();

	/**
	 * @brief Render one batch of samples. Call until rendering completes or stop() is called.
	 * @return @c true if rendering is done, @c false otherwise
	 * @sa image()
	 */
	bool update(int *currentSample = nullptr);

	/** @brief Get the cached display texture; redraw only when samples or display settings change. */
	video::TexturePtr texture();
	/** @brief Read back the display image for export. */
	image::ImagePtr image();
};

} // namespace voxelpathtracer
