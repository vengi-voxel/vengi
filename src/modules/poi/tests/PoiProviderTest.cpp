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
		_timeProvider = std::make_shared<core::TimeProvider>();
		_poiProvider = std::make_shared<PoiProvider>(_timeProvider);
	}
};

TEST_F(PoiProviderTest, testUpdate) {
	_poiProvider->add(glm::vec3(1.0));
	const poi::PoiResult& result = _poiProvider->query();
	ASSERT_TRUE(result.valid);
	ASSERT_EQ(glm::vec3(1.0), result.pos);
}

TEST_F(PoiProviderTest, testExpire) {
	_poiProvider->add(glm::vec3(1.0));
	_poiProvider->add(glm::vec3(2.0));
	_poiProvider->add(glm::vec3(3.0));
	ASSERT_EQ(3u, _poiProvider->count());
	_timeProvider->update(60 * 1000UL);
	_poiProvider->update(0UL);
	ASSERT_EQ(1u, _poiProvider->count()) << "We should have at least one poi left";
}

TEST_F(PoiProviderTest, testExpireWithProperPos) {
	const int max = 3;
	for (int i = 0; i < max; ++i) {
		_timeProvider->update(i * 60 * 1000UL);
		_poiProvider->add(glm::vec3(static_cast<float>(i)));
	}
	ASSERT_EQ(3u, _poiProvider->count());
	_poiProvider->update(0UL);
	ASSERT_EQ(1u, _poiProvider->count());
	const poi::PoiResult& result = _poiProvider->query();
	ASSERT_TRUE(result.valid);
	ASSERT_EQ(glm::vec3(static_cast<float>(max - 1)),  result.pos);
	_timeProvider->update(max * 60 * 1000UL);
	_poiProvider->update(0UL);
	ASSERT_EQ(1u, _poiProvider->count()) << "We should have at least one poi left";
}

}
