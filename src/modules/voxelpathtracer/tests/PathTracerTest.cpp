/**
 * @file
 */

#include "voxelpathtracer/PathTracer.h"
#include "app/App.h"
#include "app/tests/AbstractTest.h"
#include "image/Image.h"
#include "io/FilesystemArchive.h"
#include "io/FormatDescription.h"
#include "scenegraph/SceneGraph.h"
#include "voxel/MaterialColor.h"
#include "voxelformat/FormatConfig.h"
#include "voxelformat/VolumeFormat.h"
#include "core/GLM.h"
#include "palette/Palette.h"

class PathTracerTest : public app::AbstractTest {
private:
	using Super = app::AbstractTest;

public:
	bool onInitApp() override {
		if (!Super::onInitApp()) {
			return false;
		}
		voxelformat::FormatConfig::init();
		voxel::getPalette().nippon();
		return true;
	}
};

TEST_F(PathTracerTest, testHMec) {
	const io::ArchivePtr &archive = io::openFilesystemArchive(_testApp->filesystem());
	io::FileDescription fileDesc;
	fileDesc.set("hmec.vxl");
	scenegraph::SceneGraph sceneGraph;
	voxelformat::LoadContext testLoadCtx;
	ASSERT_TRUE(voxelformat::loadFormat(fileDesc, archive, sceneGraph, testLoadCtx))
		<< "Could not load " << fileDesc.name.c_str();

	voxelpathtracer::PathTracer pathTracer;
	pathTracer.state().params.resolution = 512;
	pathTracer.state().params.samples = 8;
	ASSERT_TRUE(pathTracer.start(sceneGraph));
	while (!pathTracer.update()) {
		_testApp->wait(100);
	}
	const image::ImagePtr &img = pathTracer.image();
	ASSERT_TRUE(img);
	ASSERT_TRUE(img->isLoaded());
	ASSERT_EQ(512, img->width());
	// ASSERT_EQ(dimensions, img->height());
	image::writeImage(img, "hmec.vxl.png");
	ASSERT_TRUE(pathTracer.stop());
}
