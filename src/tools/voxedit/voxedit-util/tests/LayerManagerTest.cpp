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
	ASSERT_EQ(0, _mgr.validLayers());
	voxel::RawVolume * v = new voxel::RawVolume(voxel::Region(0, 0));
	ASSERT_EQ(0, _mgr.addLayer("Foobar", true, v)) << "Failed to add new layer";
	ASSERT_EQ(1, _mgr.validLayers());
	delete v;
}

}
