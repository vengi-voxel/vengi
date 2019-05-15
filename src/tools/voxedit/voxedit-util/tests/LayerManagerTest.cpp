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
	std::vector<voxel::RawVolume*> _volumes;

public:
	void SetUp() override {
		Super::SetUp();
		ASSERT_TRUE(_mgr.init()) << "Failed to initialize the layer manager";
	}

	void TearDown() override {
		_mgr.shutdown();
		Super::TearDown();
		for (auto v : _volumes) {
			delete v;
		}
		_volumes.clear();
	}

	int addLayer(const char *name, bool visible = true, const voxel::Region& region = voxel::Region(0, 0)) {
		voxel::RawVolume * v = new voxel::RawVolume(region);
		_volumes.push_back(v);
		return _mgr.addLayer(name, visible, v, region.getCentre());
	}
};

TEST_F(LayerManagerTest, testValidLayersEmpty) {
	ASSERT_EQ(0, _mgr.validLayers()) << "Unexpected amount of valid layers";
}

TEST_F(LayerManagerTest, testValidLayersAfterAdd) {
	EXPECT_EQ(0, _mgr.validLayers()) << "Unexpected amount of valid layers";
	EXPECT_EQ(0, addLayer("Foobar")) << "Failed to add new layer";
	EXPECT_EQ(1, _mgr.validLayers()) << "Unexpected amount of valid layers";
}

TEST_F(LayerManagerTest, testDeleteLayer) {
	EXPECT_EQ(0, addLayer("Foobar")) << "Failed to add new layer";
	EXPECT_EQ(1, addLayer("Foobar2")) << "Failed to add new layer";
	EXPECT_EQ(2, _mgr.validLayers()) << "Unexpected amount of valid layers";
	EXPECT_TRUE(_mgr.deleteLayer(0)) << "Deleting the first layer should work";
	EXPECT_EQ(1, _mgr.validLayers()) << "Unexpected amount of valid layers";
}

TEST_F(LayerManagerTest, testDeleteLastRemainingLayer) {
	EXPECT_EQ(0, addLayer("Foobar")) << "Failed to add new layer";
	EXPECT_FALSE(_mgr.deleteLayer(0)) << "Deleting the last valid layer should not be supported";
	EXPECT_EQ(1, _mgr.validLayers()) << "Unexpected amount of valid layers";
}

TEST_F(LayerManagerTest, testMoveAfterDelete) {
	EXPECT_EQ(0, addLayer("Foobar")) << "Failed to add new layer";
	EXPECT_EQ(1, addLayer("Foobar2")) << "Failed to add new layer";
	EXPECT_EQ(2, addLayer("Foobar3")) << "Failed to add new layer";
	EXPECT_EQ(3, addLayer("Foobar4")) << "Failed to add new layer";

	EXPECT_EQ(4, _mgr.validLayers()) << "Unexpected amount of valid layers";
	EXPECT_TRUE(_mgr.deleteLayer(1)) << "Deleting the second layer should work";
	EXPECT_TRUE(_mgr.moveDown(0)) << "Moving down the first layer should work";
	EXPECT_TRUE(_mgr.layer(0).valid) << "The first (new) layer should still be valid";
	EXPECT_FALSE(_mgr.layer(1).valid) << "The second layer should still be invalid after the move";
	EXPECT_TRUE(_mgr.layer(2).valid) << "The third (new) layer should still be valid";
	EXPECT_TRUE(_mgr.layer(3).valid) << "The last (untouched) layer should still be valid";
	EXPECT_EQ(3, _mgr.validLayers()) << "Unexpected amount of valid layers";
}

}
