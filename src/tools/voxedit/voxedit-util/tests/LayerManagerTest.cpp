/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "../LayerManager.h"
#include "voxel/polyvox/RawVolume.h"

namespace voxedit {

class LayerManagerTest: public core::AbstractTest {
private:
	using Super = core::AbstractTest;
protected:
	LayerManager _mgr;

public:
	void SetUp() override {
		Super::SetUp();
		ASSERT_TRUE(_mgr.init()) << "Failed to initialize the layer manager";
	}

	void TearDown() override {
		_mgr.shutdown();
		Super::TearDown();
	}
};

TEST_F(LayerManagerTest, testValidLayersEmpty) {
	ASSERT_EQ(0, _mgr.validLayers());
}

TEST_F(LayerManagerTest, testValidLayersAfterAdd) {
	EXPECT_EQ(0, _mgr.validLayers());
	voxel::RawVolume * v = new voxel::RawVolume(voxel::Region(0, 0));
	EXPECT_EQ(0, _mgr.addLayer("Foobar", true, v)) << "Failed to add new layer";
	EXPECT_EQ(1, _mgr.validLayers());
	delete v;
}

TEST_F(LayerManagerTest, testDeleteLayer) {
	voxel::RawVolume * v = new voxel::RawVolume(voxel::Region(0, 0));
	EXPECT_EQ(0, _mgr.addLayer("Foobar", true, v)) << "Failed to add new layer";
	voxel::RawVolume * v2 = new voxel::RawVolume(voxel::Region(0, 0));
	EXPECT_EQ(1, _mgr.addLayer("Foobar2", true, v2)) << "Failed to add new layer";
	EXPECT_EQ(2, _mgr.validLayers());
	EXPECT_TRUE(_mgr.deleteLayer(0)) << "Deleting the first layer should work";
	EXPECT_EQ(1, _mgr.validLayers());
	delete v;
	delete v2;
}

TEST_F(LayerManagerTest, testDeleteLastRemainingLayer) {
	voxel::RawVolume * v = new voxel::RawVolume(voxel::Region(0, 0));
	EXPECT_EQ(0, _mgr.addLayer("Foobar", true, v)) << "Failed to add new layer";
	EXPECT_FALSE(_mgr.deleteLayer(0)) << "Deleting the last valid layer should not be supported";
	EXPECT_EQ(1, _mgr.validLayers());
	delete v;
}

TEST_F(LayerManagerTest, testMoveAfterDelete) {
	voxel::RawVolume * v1 = new voxel::RawVolume(voxel::Region(0, 0));
	EXPECT_EQ(0, _mgr.addLayer("Foobar", true, v1)) << "Failed to add new layer";
	voxel::RawVolume * v2 = new voxel::RawVolume(voxel::Region(0, 0));
	EXPECT_EQ(1, _mgr.addLayer("Foobar2", true, v2)) << "Failed to add new layer";
	voxel::RawVolume * v3 = new voxel::RawVolume(voxel::Region(0, 0));
	EXPECT_EQ(2, _mgr.addLayer("Foobar3", true, v3)) << "Failed to add new layer";
	voxel::RawVolume * v4 = new voxel::RawVolume(voxel::Region(0, 0));
	EXPECT_EQ(3, _mgr.addLayer("Foobar4", true, v4)) << "Failed to add new layer";

	EXPECT_EQ(4, _mgr.validLayers());
	EXPECT_TRUE(_mgr.deleteLayer(1)) << "Deleting the second layer should work";
	EXPECT_TRUE(_mgr.moveDown(0));
	EXPECT_TRUE(_mgr.layer(0).valid);
	EXPECT_FALSE(_mgr.layer(1).valid);
	EXPECT_TRUE(_mgr.layer(2).valid);
	EXPECT_TRUE(_mgr.layer(3).valid);
	EXPECT_EQ(3, _mgr.validLayers());
	delete v1;
	delete v2;
	delete v3;
	delete v4;
}

}
