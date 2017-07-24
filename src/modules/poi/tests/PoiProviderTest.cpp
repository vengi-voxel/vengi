/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "poi/PoiProvider.h"
#include "voxel/World.h"

namespace poi {

class PoiProviderTest: public core::AbstractTest {
public:
	PoiProviderPtr _poiProvider;
	core::TimeProviderPtr _timeProvider;
	void SetUp() override {
		core::AbstractTest::SetUp();
		io::FilesystemPtr filesystem(new io::Filesystem());
		voxel::WorldPtr world = std::make_shared<voxel::World>();
		_timeProvider = std::make_shared<core::TimeProvider>();
		_poiProvider = std::make_shared<PoiProvider>(world, _timeProvider);
	}
};

TEST_F(PoiProviderTest, testUpdate) {
	_poiProvider->addPointOfInterest(glm::vec3(1.0));
	ASSERT_EQ(glm::vec3(1.0), _poiProvider->getPointOfInterest());
}

TEST_F(PoiProviderTest, testExpire) {
	_poiProvider->addPointOfInterest(glm::vec3(1.0));
	_poiProvider->addPointOfInterest(glm::vec3(2.0));
	_poiProvider->addPointOfInterest(glm::vec3(3.0));
	ASSERT_EQ(3u, _poiProvider->getPointOfInterestCount());
	_timeProvider->update(60 * 1000UL);
	_poiProvider->update(0UL);
	ASSERT_EQ(0u, _poiProvider->getPointOfInterestCount());
}

TEST_F(PoiProviderTest, testExpireWithProperPos) {
	const int max = 3;
	for (int i = 0; i < max; ++i) {
		_timeProvider->update(i * 60 * 1000UL);
		_poiProvider->addPointOfInterest(glm::vec3(static_cast<float>(i)));
	}
	ASSERT_EQ(3u, _poiProvider->getPointOfInterestCount());
	_poiProvider->update(0UL);
	ASSERT_EQ(1u, _poiProvider->getPointOfInterestCount());
	ASSERT_EQ(glm::vec3(static_cast<float>(max - 1)), _poiProvider->getPointOfInterest());
	_timeProvider->update(max * 60 * 1000UL);
	_poiProvider->update(0UL);
	ASSERT_EQ(0u, _poiProvider->getPointOfInterestCount());
}

}
