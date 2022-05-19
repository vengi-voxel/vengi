/**
 * @file
 */

#include "app/App.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "app/tests/AbstractTest.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Voxel.h"
#include "voxel/tests/TestHelper.h"
#include "voxelformat/QBFormat.h"
#include "voxelformat/SceneGraph.h"
#include "voxelgenerator/ShapeGenerator.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelgenerator {

class ShapeGeneratorTest: public app::AbstractTest {
private:
	using Super = app::AbstractTest;
protected:
	static const voxel::Region _region;
	static constexpr glm::ivec3 _center { 15 };
	static constexpr int _width { 32 };
	static constexpr int _height { 32 };
	static constexpr int _depth { 32 };
	static constexpr voxel::Voxel _voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	voxel::RawVolume *_volume = nullptr;

	void verify(const char* filename) {
		voxelformat::QBFormat format;
		const io::FilePtr& file = io::filesystem()->open(filename);
		ASSERT_TRUE(file) << "Can't open " << filename;
		io::FileStream stream(file);
		voxelformat::SceneGraph sceneGraph;
		ASSERT_TRUE(format.loadGroups(file->fileName(), stream, sceneGraph));
		voxelformat::SceneGraph::MergedVolumePalette merged = sceneGraph.merge();
		voxel::RawVolume* v = merged.first;
		ASSERT_NE(nullptr, v) << "Can't load " << filename;
		EXPECT_TRUE(volumeComparator(v, *_volume, true, true)) << "Volumes differ: " << v << *_volume;
		delete v;
	}
public:
	void SetUp() override {
		Super::SetUp();
		voxel::initDefaultPalette();
		_volume = new voxel::RawVolume(_region);
	}

	void TearDown() override {
		delete _volume;
		Super::TearDown();
	}
};

const voxel::Region ShapeGeneratorTest::_region = voxel::Region{0, 31};

TEST_F(ShapeGeneratorTest, testCreateCubeNoCenter) {
	voxel::RawVolumeWrapper wrapper(_volume);
	shape::createCubeNoCenter(wrapper, _region.getLowerCorner(), _width, _height, _depth, _voxel);
	verify("cube.qb");
}

TEST_F(ShapeGeneratorTest, testCreateCube) {
	voxel::RawVolumeWrapper wrapper(_volume);
	shape::createCube(wrapper, _center, _width, _height, _depth, _voxel);
	int count = 0;
	voxelutil::visitVolume(*_volume, [&count] (int x, int y, int z, const voxel::Voxel& voxel) {
		++count;
	});
	// this -1 is due to rounding errors - but those are expected. The shape generator doesn't
	// know anything about the region position.
	ASSERT_EQ((_width - 1) * (_height - 1) * (_depth - 1), count);
}

TEST_F(ShapeGeneratorTest, testCreateEllipse) {
	voxel::RawVolumeWrapper wrapper(_volume);
	shape::createEllipse(wrapper, _center, _width, _height, _depth, _voxel);
	verify("ellipse.qb");
}

TEST_F(ShapeGeneratorTest, testCreateCone) {
	voxel::RawVolumeWrapper wrapper(_volume);
	shape::createCone(wrapper, _center, _width, _height, _depth, _voxel);
	verify("cone.qb");
}

TEST_F(ShapeGeneratorTest, testCreateDome) {
	voxel::RawVolumeWrapper wrapper(_volume);
	shape::createDome(wrapper, _center, _width, _height, _depth, _voxel);
	verify("dome.qb");
}

TEST_F(ShapeGeneratorTest, testCreateCylinder) {
	voxel::RawVolumeWrapper wrapper(_volume);
	glm::ivec3 centerBottom = _center;
	centerBottom.y = _region.getLowerY();
	shape::createCylinder(wrapper, centerBottom, math::Axis::Y, _width / 2, _height, _voxel);
	verify("cylinder.qb");
}

}
