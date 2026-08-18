/**
 * @file
 */

#include "TestVolumeRenderer.h"
#include "color/RGBA.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/Var.h"
#include "io/Filesystem.h"
#include "io/FilesystemArchive.h"
#include "io/FormatDescription.h"
#include "math/Math.h"
#include "palette/Material.h"
#include "scenegraph/FrameTransform.h"
#include "scenegraph/SceneGraphNode.h"
#include "testcore/TestAppMain.h"
#include "video/Renderer.h"
#include "video/Types.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelformat/Format.h"
#include "voxelformat/FormatConfig.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelrender/RenderUtil.h"
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtc/matrix_transform.hpp>

namespace {

void fillBox(voxel::RawVolume *volume, const voxel::Region &box, const voxel::Voxel &voxel) {
	for (int z = box.getLowerZ(); z <= box.getUpperZ(); ++z) {
		for (int y = box.getLowerY(); y <= box.getUpperY(); ++y) {
			for (int x = box.getLowerX(); x <= box.getUpperX(); ++x) {
				volume->setVoxel(x, y, z, voxel);
			}
		}
	}
}

} // namespace

TestVolumeRenderer::TestVolumeRenderer(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider)
	: Super(filesystem, timeProvider), _renderer(timeProvider) {
	init(ORGANISATION, "testvolumerenderer");
	setCameraMotion(true);
	// Voxel ground receives shadows from the volume renderer; the TestApp plane does not.
	setRenderPlane(false);
	setRenderAxis(true);
}

app::AppState TestVolumeRenderer::onConstruct() {
	app::AppState state = Super::onConstruct();
	voxelformat::FormatConfig::init();
	registerArg("--input")
		.setShort("-i")
		.setDescription("Optional voxel model to load instead of the built-in material scene")
		.addFlag(ARGUMENT_FLAG_FILE);

	core::getVar(cfg::ClientBloom)->setVal(true);
	core::getVar(cfg::ClientShadowMap)->setVal(true);
	core::getVar(cfg::RenderCheckerBoard)->setVal(true);
	core::getVar(cfg::RenderOutline)->setVal(true);
	core::getVar(cfg::RenderToneMapping)->setVal(1);
	return state;
}

void TestVolumeRenderer::configureRendererFeatures() {
	_renderer.setAmbientColor(glm::vec3(0.25f, 0.26f, 0.30f));
	_renderer.setDiffuseColor(glm::vec3(0.85f, 0.80f, 0.70f));
	_renderer.setSunAngle(glm::vec3(32.0f, 50.0f, 0.0f));
}

bool TestVolumeRenderer::addVolume(int idx, voxel::RawVolume *volume, palette::Palette *palette,
								  const glm::mat4 &worldMatrix) {
	delete _renderer.setVolume(_meshState, idx, volume, palette, nullptr, true);
	const voxel::Region &region = volume->region();
	glm::vec3 mins;
	glm::vec3 maxs;
	region.transformArvo(worldMatrix, mins, maxs);
	_meshState->setCullFace(idx, math::det3x3(worldMatrix) < 0.0f ? video::Face::Front : video::Face::Back);
	if (!_meshState->setModelMatrix(idx, worldMatrix, mins, maxs)) {
		Log::error("Failed to set model matrix for volume %i", idx);
		return false;
	}
	_renderer.scheduleRegionExtraction(_meshState, idx, region);
	return true;
}

void TestVolumeRenderer::flushMeshes() {
	_meshState->extractAllPending();
	for (int i = 0; i < 64 && _meshState->pendingMeshes() > 0; ++i) {
		_renderer.update(_meshState);
	}
	if (_meshState->pendingMeshes() > 0) {
		Log::warn("Mesh uploads still pending after flush (%i)", _meshState->pendingMeshes());
	}
}

bool TestVolumeRenderer::createMaterialScene() {
	_ownsVolumes = true;
	_palette.setSize(7);
	_palette.setColor(0, color::RGBA(0, 0, 0, 0));

	_palette.setColor(1, color::RGBA(200, 50, 50));
	_palette.setMaterialType(1, palette::MaterialType::Diffuse);
	_palette.setRoughness(1, 0.85f);

	_palette.setColor(2, color::RGBA(180, 180, 195));
	_palette.setMaterialType(2, palette::MaterialType::Metal);
	_palette.setMetal(2, 1.0f);
	_palette.setRoughness(2, 0.15f);
	_palette.setSpecular(2, 0.9f);

	_palette.setColor(3, color::RGBA(80, 180, 220, 96));
	_palette.setMaterialType(3, palette::MaterialType::Glass);
	_palette.setIndexOfRefraction(3, 1.5f);

	_palette.setColor(4, color::RGBA(255, 180, 40));
	_palette.setMaterialType(4, palette::MaterialType::Emit);
	_palette.setEmit(4, 1.0f);
	_palette.setFlux(4, 1.0f);

	_palette.setColor(5, color::RGBA(70, 70, 75));
	_palette.setMaterialType(5, palette::MaterialType::Diffuse);
	_palette.setRoughness(5, 1.0f);

	_palette.setColor(6, color::RGBA(255, 230, 80));
	_palette.setMaterialType(6, palette::MaterialType::Diffuse);

	const voxel::Region region(-8, -2, -8, 84, 24, 24);
	voxel::RawVolume *volume = new voxel::RawVolume(region);

	fillBox(volume, voxel::Region(-8, -2, -8, 84, -1, 24), voxel::createVoxel(_palette, 5));
	// Tall wall on the +X side. Sun is from +X/+Z so the shadow falls -X across the floor.
	fillBox(volume, voxel::Region(78, 0, -6, 80, 22, 20), voxel::createVoxel(_palette, 1));
	fillBox(volume, voxel::Region(0, 0, 0, 11, 11, 11), voxel::createVoxel(_palette, 1));
	fillBox(volume, voxel::Region(20, 0, 0, 31, 11, 11), voxel::createVoxel(_palette, 2));
	fillBox(volume, voxel::Region(40, 0, 0, 51, 11, 11), voxel::createVoxel(_palette, 3));
	fillBox(volume, voxel::Region(60, 0, 0, 71, 11, 11), voxel::createVoxel(_palette, 4));

	voxel::Voxel outlineVoxel = voxel::createVoxel(_palette, 6);
	outlineVoxel.setOutline();
	fillBox(volume, voxel::Region(2, 12, 2, 9, 13, 9), outlineVoxel);

	if (!addVolume(0, volume, &_palette, glm::mat4(1.0f))) {
		return false;
	}

	voxelrender::configureCamera(camera(), region, voxelrender::SceneCameraMode::Free, 500.0f);
	camera().update(0.0);
	return true;
}

bool TestVolumeRenderer::loadScene(const core::String &filename) {
	io::FileDescription fileDesc;
	fileDesc.set(filename);
	const io::ArchivePtr &archive = io::openFilesystemArchive(_filesystem);
	voxelformat::LoadContext loadCtx;
	if (!voxelformat::loadFormat(fileDesc, archive, _sceneGraph, loadCtx)) {
		Log::error("Failed to load voxel model '%s'", filename.c_str());
		return false;
	}

	_ownsVolumes = false;
	int idx = 0;
	for (auto iter = _sceneGraph.beginModel(); iter != _sceneGraph.end(); ++iter) {
		scenegraph::SceneGraphNode &node = *iter;
		voxel::RawVolume *volume = node.volume();
		if (volume == nullptr) {
			continue;
		}
		const scenegraph::FrameTransform frameTransform = _sceneGraph.transformForFrame(node, 0);
		const glm::mat4 &wm = frameTransform.worldMatrix();
		const voxel::Region &region = volume->region();
		const glm::vec3 dimensions(region.getDimensionsInVoxels());
		const glm::mat4 worldMatrix = glm::translate(wm, -node.pivot() * dimensions);
		if (!addVolume(idx, volume, &node.palette(), worldMatrix)) {
			return false;
		}
		++idx;
	}
	if (idx == 0) {
		Log::error("No model nodes found in '%s'", filename.c_str());
		return false;
	}

	voxelrender::configureCamera(camera(), _sceneGraph.sceneRegion(), voxelrender::SceneCameraMode::Free, 500.0f);
	camera().update(0.0);
	return true;
}

app::AppState TestVolumeRenderer::onInit() {
	app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

	_meshState = core::make_shared<voxel::MeshState>();
	_meshState->construct();
	if (!_meshState->init()) {
		Log::error("Failed to init mesh state");
		return app::AppState::InitFailure;
	}

	_renderer.construct();
	if (!_renderer.init(_meshState->hasNormals())) {
		Log::error("Failed to init raw volume renderer");
		return app::AppState::InitFailure;
	}

	if (!_renderContext.init(frameBufferDimension())) {
		Log::error("Failed to init render context");
		return app::AppState::InitFailure;
	}
	_renderContext.renderMode = voxelrender::RenderMode::Scene;
	configureRendererFeatures();

	if (hasArg("--input")) {
		const core::String filename = getArgVal("--input");
		if (!loadScene(filename)) {
			return app::AppState::InitFailure;
		}
	} else if (!createMaterialScene()) {
		Log::error("Failed to create the material scene");
		return app::AppState::InitFailure;
	}

	flushMeshes();
	return state;
}

void TestVolumeRenderer::presentFrameBuffer() {
	const glm::ivec2 dim = _renderContext.frameBuffer.dimension();
	video::Id src = _renderContext.frameBuffer.handle();
	if (_renderContext.enableMultisampling) {
		src = _renderContext.resolveFrameBuffer.handle();
	}
	video::blitFramebuffer(src, video::InvalidId, video::ClearFlag::Color, dim.x, dim.y);
}

void TestVolumeRenderer::doRender() {
	_renderer.update(_meshState);
	_renderContext.frameBuffer.bind(true);
	_renderer.render(_meshState, _renderContext, camera(), true, true);
	if (_renderContext.enableMultisampling) {
		const glm::ivec2 dim = _renderContext.frameBuffer.dimension();
		video::blitFramebuffer(_renderContext.frameBuffer.handle(), _renderContext.resolveFrameBuffer.handle(),
							   video::ClearFlag::Color, dim.x, dim.y);
	}
	_renderContext.frameBuffer.unbind();
	presentFrameBuffer();
}

void TestVolumeRenderer::onWindowResize(void *windowHandle, int windowWidth, int windowHeight) {
	Super::onWindowResize(windowHandle, windowWidth, windowHeight);
	if (_renderContext.frameBuffer.handle() != video::InvalidId) {
		_renderContext.resize(frameBufferDimension());
	}
}

app::AppState TestVolumeRenderer::onCleanup() {
	if (_meshState) {
		// clear() resets each slot and drops the pointer; delete owned volumes first.
		if (_ownsVolumes) {
			const core::Buffer<int> active = _meshState->activeIndices();
			for (int i : active) {
				delete _renderer.resetVolume(_meshState, i);
			}
		}
		_renderer.clear(_meshState);
		(void)_meshState->shutdown();
	}
	_renderer.shutdown();
	_renderContext.shutdown();
	return Super::onCleanup();
}

TEST_APP(TestVolumeRenderer)
