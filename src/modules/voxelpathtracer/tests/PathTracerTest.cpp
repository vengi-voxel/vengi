/**
 * @file
 */

#include "voxelpathtracer/PathTracer.h"
#include "color/RGBA.h"
#include "image/Image.h"
#include "io/FileStream.h"
#include "io/FilesystemArchive.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "video/Camera.h"
#include "video/tests/AbstractGLTest.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelformat/FormatConfig.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelpathtracer/PathTracerState.h"
#include "voxelrender/RenderUtil.h"

namespace {

void fillRegion(voxel::RawVolume *v, const palette::Palette &pal, uint8_t index, const voxel::Region &region) {
	for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
		for (int y = region.getLowerY(); y <= region.getUpperY(); ++y) {
			for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
				v->setVoxel(x, y, z, voxel::createVoxel(pal, index));
			}
		}
	}
}

void fillSlabXY(voxel::RawVolume *v, const voxel::Voxel &voxel, int z, int maxXY) {
	for (int y = 0; y <= maxXY; ++y) {
		for (int x = 0; x <= maxXY; ++x) {
			v->setVoxel(x, y, z, voxel);
		}
	}
}

void configureDarkParams(voxelpathtracer::PathTracerParams &params, int resolution, int samples, int samplesThisPass,
						 int bounces) {
	params.resolution = resolution;
	params.samples = samples;
	params.samplesThisPass = samplesThisPass;
	params.bounces = bounces;
	params.floorEnabled = false;
	params.vignette = 0.0f;
	params.sunIntensity = 0.0f;
	params.skyIntensity = 0.0f;
	params.background = voxelpathtracer::PathTracerBackgroundSolid;
	params.backgroundColor = glm::vec3(0.0f);
}

video::Camera frontCamera(const voxel::Region &region, int size, float farPlane) {
	video::Camera camera;
	camera.setSize(glm::ivec2(size, size));
	voxelrender::configureCamera(camera, region, voxelrender::SceneCameraMode::Front, farPlane);
	camera.update(0.0);
	return camera;
}

void renderUntilDone(voxelpathtracer::PathTracer &pathTracer) {
	while (!pathTracer.update()) {
	}
}

scenegraph::SceneGraphNode makeModelNode(const char *name, voxel::RawVolume *volume, const palette::Palette &pal) {
	scenegraph::SceneGraphNode model(scenegraph::SceneGraphNodeType::Model);
	model.setName(name);
	model.setVolume(volume);
	model.setPalette(pal);
	return model;
}

bool loadHmec(const io::FilesystemPtr &fs, scenegraph::SceneGraph &sceneGraph) {
	const io::ArchivePtr &archive = io::openFilesystemArchive(fs);
	io::FileDescription fileDesc;
	fileDesc.set("hmec.vxl");
	voxelformat::LoadContext testLoadCtx;
	return voxelformat::loadFormat(fileDesc, archive, sceneGraph, testLoadCtx);
}

enum class TransmissionCase {
	PaletteAlpha,
	VoxelTypeTransparent,
	Glass,
	Media
};

struct TransmissionParam {
	TransmissionCase kind;
	const char *name;
	int minRed;
};

void setupTransmissionSlab(palette::Palette &pal, TransmissionCase kind) {
	pal.setColor(1, color::RGBA(255, 16, 16, 255));
	pal.setEmit(1, 1.0f);
	switch (kind) {
	case TransmissionCase::PaletteAlpha:
		pal.setColor(2, color::RGBA(255, 255, 255, 32));
		break;
	case TransmissionCase::VoxelTypeTransparent:
		pal.setColor(2, color::RGBA(255, 255, 255, 255));
		break;
	case TransmissionCase::Glass:
		pal.setColor(2, color::RGBA(220, 240, 255, 255));
		pal.setMaterialType(2, palette::MaterialType::Glass);
		pal.setIndexOfRefraction(2, 1.3f);
		pal.setAttenuation(2, 0.1f);
		break;
	case TransmissionCase::Media:
		pal.setColor(2, color::RGBA(200, 220, 255, 255));
		pal.setMaterialType(2, palette::MaterialType::Media);
		pal.setDensity(2, 0.2f);
		pal.setPhase(2, 0.5f);
		pal.setMedia(2, 1.0f);
		break;
	}
}

voxel::Voxel transmissionSlabVoxel(const palette::Palette &pal, TransmissionCase kind) {
	if (kind == TransmissionCase::VoxelTypeTransparent) {
		return voxel::createVoxel(voxel::VoxelType::Transparent, 2);
	}
	return voxel::createVoxel(pal, 2);
}

} // namespace

class PathTracerTest : public video::AbstractGLTest {
private:
	using Super = video::AbstractGLTest;

public:
	void SetUp() override {
		Super::SetUp();
		if (IsSkipped()) {
			return;
		}
		voxelformat::FormatConfig::init();
	}
};

class PathTracerTransmissionTest : public PathTracerTest,
								   public ::testing::WithParamInterface<TransmissionParam> {};

TEST_P(PathTracerTransmissionTest, showsEmissiveVoxelBehind) {
	const TransmissionParam &param = GetParam();
	scenegraph::SceneGraph sceneGraph;
	voxel::RawVolume *v = new voxel::RawVolume(voxel::Region(0, 0, 0, 7, 7, 7));
	palette::Palette pal;
	setupTransmissionSlab(pal, param.kind);
	fillSlabXY(v, transmissionSlabVoxel(pal, param.kind), 1, 7);
	fillSlabXY(v, voxel::createVoxel(pal, 1), 6, 7);
	ASSERT_NE(InvalidNodeId, sceneGraph.emplace(makeModelNode("model", v, pal)));
	sceneGraph.updateTransforms();

	video::Camera camera = frontCamera(sceneGraph.sceneRegion(), 64, 64.0f);
	voxelpathtracer::PathTracer pathTracer;
	configureDarkParams(pathTracer.state().params, 64, 32, 8, 6);
	ASSERT_TRUE(pathTracer.start(sceneGraph, &camera));
	EXPECT_TRUE(pathTracer.hasVolume());
	renderUntilDone(pathTracer);
	const image::ImagePtr &img = pathTracer.image();
	ASSERT_TRUE(img && img->isLoaded());
	const color::RGBA center = img->colorAt(32, 32);
	EXPECT_GT((int)center.r, param.minRed) << param.name << " got " << (int)center.r << "," << (int)center.g << ","
										   << (int)center.b;
	EXPECT_GT((int)center.r, (int)center.b) << param.name << " expected red-dominant center";
	ASSERT_TRUE(pathTracer.stop());
}

INSTANTIATE_TEST_SUITE_P(
	Transmission, PathTracerTransmissionTest,
	::testing::Values(TransmissionParam{TransmissionCase::PaletteAlpha, "palette_alpha", 80},
					  TransmissionParam{TransmissionCase::VoxelTypeTransparent, "voxel_type_transparent", 80},
					  TransmissionParam{TransmissionCase::Glass, "glass", 80},
					  TransmissionParam{TransmissionCase::Media, "media", 40}),
	[](const testing::TestParamInfo<TransmissionParam> &nfo) { return nfo.param.name; });

TEST_F(PathTracerTest, testEmitFluxBrightensVoxel) {
	scenegraph::SceneGraph sceneGraph;
	voxel::RawVolume *v = new voxel::RawVolume(voxel::Region(0, 0, 0, 3, 3, 3));
	palette::Palette pal;
	pal.setColor(1, color::RGBA(255, 32, 16, 255));
	pal.setMaterialType(1, palette::MaterialType::Emit);
	pal.setEmit(1, 0.25f);
	pal.setFlux(1, 3.0f);
	fillRegion(v, pal, 1, voxel::Region(0, 0, 0, 3, 3, 3));
	ASSERT_NE(InvalidNodeId, sceneGraph.emplace(makeModelNode("model", v, pal)));
	sceneGraph.updateTransforms();

	video::Camera camera = frontCamera(sceneGraph.sceneRegion(), 64, 64.0f);
	voxelpathtracer::PathTracer pathTracer;
	configureDarkParams(pathTracer.state().params, 64, 16, 8, 2);
	ASSERT_TRUE(pathTracer.start(sceneGraph, &camera));
	renderUntilDone(pathTracer);
	const image::ImagePtr &img = pathTracer.image();
	ASSERT_TRUE(img && img->isLoaded());
	EXPECT_GT((int)img->colorAt(32, 32).r, 120);
	ASSERT_TRUE(pathTracer.stop());
}

TEST_F(PathTracerTest, testHMec) {
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(loadHmec(_testApp->filesystem(), sceneGraph)) << "Could not load hmec.vxl";

	voxelpathtracer::PathTracer pathTracer;
	pathTracer.state().params.resolution = 64;
	pathTracer.state().params.samples = 2;
	pathTracer.state().params.samplesThisPass = 1;
	pathTracer.state().params.bounces = 2;
	ASSERT_TRUE(pathTracer.start(sceneGraph));
	renderUntilDone(pathTracer);
	const image::ImagePtr &img = pathTracer.image();
	ASSERT_TRUE(img && img->isLoaded());
	ASSERT_EQ(64, img->width());
	ASSERT_EQ(64, img->height());
	const io::FilePtr &file = _testApp->filesystem()->open("hmec.vxl.png", io::FileMode::SysWrite);
	io::FileStream stream(file);
	EXPECT_TRUE(image::writePNG(img, stream));
	ASSERT_TRUE(pathTracer.stop());
}

TEST_F(PathTracerTest, testPaletteColorChangeUpdatesRender) {
	scenegraph::SceneGraph sceneGraph;
	voxel::RawVolume *v = new voxel::RawVolume(voxel::Region(0, 0, 0, 3, 3, 3));
	palette::Palette pal;
	pal.setColor(1, color::RGBA(255, 16, 16, 255));
	pal.setEmit(1, 1.0f);
	fillRegion(v, pal, 1, voxel::Region(0, 0, 0, 3, 3, 3));
	const int nodeId = sceneGraph.emplace(makeModelNode("model", v, pal));
	ASSERT_NE(InvalidNodeId, nodeId);
	sceneGraph.updateTransforms();

	video::Camera camera = frontCamera(sceneGraph.sceneRegion(), 64, 64.0f);
	voxelpathtracer::PathTracer pathTracer;
	configureDarkParams(pathTracer.state().params, 64, 16, 8, 2);
	ASSERT_TRUE(pathTracer.start(sceneGraph, &camera));
	renderUntilDone(pathTracer);
	const color::RGBA redCenter = pathTracer.image()->colorAt(32, 32);
	EXPECT_GT((int)redCenter.r, (int)redCenter.b);

	sceneGraph.node(nodeId).palette().setColor(1, color::RGBA(16, 16, 255, 255));
	ASSERT_TRUE(pathTracer.start(sceneGraph, &camera));
	renderUntilDone(pathTracer);
	const color::RGBA blueCenter = pathTracer.image()->colorAt(32, 32);
	EXPECT_GT((int)blueCenter.b, (int)blueCenter.r);
	ASSERT_TRUE(pathTracer.stop());
}

TEST_F(PathTracerTest, testPerNodePalettesKeepDistinctColors) {
	scenegraph::SceneGraph sceneGraph;

	voxel::RawVolume *redVol = new voxel::RawVolume(voxel::Region(0, 0, 0, 3, 3, 3));
	palette::Palette redPal;
	redPal.setColor(1, color::RGBA(255, 16, 16, 255));
	redPal.setEmit(1, 1.0f);
	fillRegion(redVol, redPal, 1, voxel::Region(0, 0, 0, 3, 3, 3));
	ASSERT_NE(InvalidNodeId, sceneGraph.emplace(makeModelNode("red", redVol, redPal)));

	voxel::RawVolume *blueVol = new voxel::RawVolume(voxel::Region(8, 0, 0, 11, 3, 3));
	palette::Palette bluePal;
	bluePal.setColor(1, color::RGBA(16, 16, 255, 255));
	bluePal.setEmit(1, 1.0f);
	fillRegion(blueVol, bluePal, 1, voxel::Region(8, 0, 0, 11, 3, 3));
	ASSERT_NE(InvalidNodeId, sceneGraph.emplace(makeModelNode("blue", blueVol, bluePal)));
	sceneGraph.updateTransforms();

	video::Camera camera = frontCamera(sceneGraph.sceneRegion(), 64, 64.0f);
	voxelpathtracer::PathTracer pathTracer;
	configureDarkParams(pathTracer.state().params, 64, 16, 8, 2);
	ASSERT_TRUE(pathTracer.start(sceneGraph, &camera));
	EXPECT_TRUE(pathTracer.hasVolume());
	renderUntilDone(pathTracer);
	const image::ImagePtr &img = pathTracer.image();
	ASSERT_TRUE(img && img->isLoaded());
	int maxR = 0;
	int maxB = 0;
	int maxRX = 0;
	int maxBX = 0;
	for (int y = 0; y < img->height(); ++y) {
		for (int x = 0; x < img->width(); ++x) {
			const color::RGBA c = img->colorAt(x, y);
			if ((int)c.r > maxR) {
				maxR = (int)c.r;
				maxRX = x;
			}
			if ((int)c.b > maxB) {
				maxB = (int)c.b;
				maxBX = x;
			}
		}
	}
	EXPECT_GT(maxR, 80);
	EXPECT_GT(maxB, 80);
	EXPECT_NE(maxRX, maxBX);
	ASSERT_TRUE(pathTracer.stop());
}

TEST_F(PathTracerTest, testRenderScaleHalfResolution) {
	scenegraph::SceneGraph sceneGraph;
	voxel::RawVolume *v = new voxel::RawVolume(voxel::Region(0, 0, 0, 3, 3, 3));
	palette::Palette pal;
	pal.setColor(1, color::RGBA(255, 255, 255, 255));
	pal.setEmit(1, 1.0f);
	v->setVoxel(1, 1, 1, voxel::createVoxel(pal, 1));
	ASSERT_NE(InvalidNodeId, sceneGraph.emplace(makeModelNode("cube", v, pal)));
	sceneGraph.updateTransforms();

	voxelpathtracer::PathTracer pathTracer;
	pathTracer.state().params.resolution = 64;
	pathTracer.state().params.renderScale = 0.5f;
	pathTracer.state().params.samples = 1;
	pathTracer.state().params.samplesThisPass = 1;
	pathTracer.state().params.bounces = 1;
	ASSERT_TRUE(pathTracer.start(sceneGraph));
	renderUntilDone(pathTracer);
	const image::ImagePtr &img = pathTracer.image();
	ASSERT_TRUE(img && img->isLoaded());
	EXPECT_EQ(32, img->width());
	EXPECT_EQ(32, img->height());
	ASSERT_TRUE(pathTracer.stop());
}

TEST_F(PathTracerTest, testTonemapAgxDiffersFromNone) {
	scenegraph::SceneGraph sceneGraph;
	voxel::RawVolume *v = new voxel::RawVolume(voxel::Region(0, 0, 0, 3, 3, 3));
	palette::Palette pal;
	pal.setColor(1, color::RGBA(255, 255, 255, 255));
	pal.setEmit(1, 1.0f);
	fillRegion(v, pal, 1, voxel::Region(1, 1, 1, 2, 2, 2));
	ASSERT_NE(InvalidNodeId, sceneGraph.emplace(makeModelNode("cube", v, pal)));
	sceneGraph.updateTransforms();

	video::Camera camera = frontCamera(sceneGraph.sceneRegion(), 64, 32.0f);
	voxelpathtracer::PathTracer pathTracer;
	configureDarkParams(pathTracer.state().params, 64, 8, 8, 1);
	pathTracer.state().params.toneMapping = voxelrender::ToneMappingNone;
	ASSERT_TRUE(pathTracer.start(sceneGraph, &camera));
	renderUntilDone(pathTracer);
	const color::RGBA noneCenter = pathTracer.image()->colorAt(32, 32);

	pathTracer.state().params.toneMapping = voxelrender::ToneMappingAgx;
	const color::RGBA agxCenter = pathTracer.image()->colorAt(32, 32);
	EXPECT_NE(noneCenter.r, agxCenter.r);
	EXPECT_GT((int)agxCenter.r, 0);
	ASSERT_TRUE(pathTracer.stop());
}

TEST_F(PathTracerTest, testApplyCameraResetsAccumulation) {
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(loadHmec(_testApp->filesystem(), sceneGraph)) << "Could not load hmec.vxl";

	video::Camera camera;
	camera.setSize(glm::ivec2(64));
	voxelrender::configureCamera(camera, sceneGraph.sceneRegion(), voxelrender::SceneCameraMode::Free, 500.0f);
	camera.update(0.0);

	voxelpathtracer::PathTracer pathTracer;
	pathTracer.state().params.resolution = 64;
	pathTracer.state().params.samples = 8;
	pathTracer.state().params.samplesThisPass = 1;
	pathTracer.state().params.bounces = 2;
	ASSERT_TRUE(pathTracer.start(sceneGraph, &camera));
	int currentSample = 0;
	ASSERT_FALSE(pathTracer.update(&currentSample));
	ASSERT_GT(currentSample, 0);

	pathTracer.applyCamera(&camera);
	EXPECT_EQ(currentSample, pathTracer.state().currentSample);

	camera.rotate(0.4f, glm::vec3(0.0f, 1.0f, 0.0f));
	camera.update(0.0);
	pathTracer.applyCamera(&camera);
	EXPECT_EQ(0, pathTracer.state().currentSample);
	EXPECT_TRUE(pathTracer.started());
	ASSERT_TRUE(pathTracer.stop());
}

TEST_F(PathTracerTest, testStartReplacesLoadedScene) {
	scenegraph::SceneGraph emptyGraph;
	voxelpathtracer::PathTracer pathTracer;
	pathTracer.state().params.resolution = 64;
	pathTracer.state().params.samples = 2;
	pathTracer.state().params.samplesThisPass = 1;
	pathTracer.state().params.bounces = 2;
	ASSERT_TRUE(pathTracer.start(emptyGraph));
	EXPECT_FALSE(pathTracer.hasVolume());

	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(loadHmec(_testApp->filesystem(), sceneGraph)) << "Could not load hmec.vxl";
	ASSERT_TRUE(pathTracer.start(sceneGraph));
	EXPECT_TRUE(pathTracer.hasVolume());
	ASSERT_TRUE(pathTracer.stop());
}

TEST_F(PathTracerTest, testGpuDisplayTextureSupportsExportAndResize) {
	scenegraph::SceneGraph sceneGraph;
	voxelpathtracer::PathTracer pathTracer;
	auto &params = pathTracer.state().params;
	params.resolution = 32;
	params.samples = 1;
	params.floorEnabled = false;
	params.background = voxelpathtracer::PathTracerBackgroundSolid;
	params.backgroundColor = glm::vec3(1.0f, 0.0f, 0.0f);
	params.vignette = 0.0f;
	EXPECT_FALSE(pathTracer.texture());
	ASSERT_TRUE(pathTracer.start(sceneGraph));
	ASSERT_TRUE(pathTracer.update());
	const video::TexturePtr preview = pathTracer.texture();
	ASSERT_TRUE(preview);
	EXPECT_TRUE(preview->isLoaded());
	EXPECT_EQ(32, preview->width());
	EXPECT_EQ(32, preview->height());
	EXPECT_EQ(preview->handle(), pathTracer.texture()->handle());
	const image::ImagePtr exported = pathTracer.image();
	ASSERT_TRUE(exported);
	EXPECT_GT((int)exported->colorAt(16, 16).r, 80);
	EXPECT_EQ(0, (int)exported->colorAt(16, 16).b);
	params.resolution = 64;
	pathTracer.resetAccumulation();
	ASSERT_TRUE(pathTracer.update());
	EXPECT_EQ(64, pathTracer.texture()->width());
	EXPECT_EQ(64, pathTracer.texture()->height());
	ASSERT_TRUE(pathTracer.stop());
}
