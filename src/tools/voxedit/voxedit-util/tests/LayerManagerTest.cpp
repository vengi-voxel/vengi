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

}
