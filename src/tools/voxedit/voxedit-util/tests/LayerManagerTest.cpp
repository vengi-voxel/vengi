/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "../LayerManager.h"

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
	ASSERT_EQ(0, _mgr.addLayer("Foobar", true, nullptr)) << "Failed to add new layer";
	ASSERT_EQ(1, _mgr.validLayers());
}

}
