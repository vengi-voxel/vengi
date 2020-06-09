/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "poi/PoiProvider.h"
#include "voxelworld/WorldMgr.h"

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
	EXPECT_TRUE(result.valid);
	EXPECT_EQ(glm::vec3(1.0), result.pos);
}

TEST_F(PoiProviderTest, testExpire) {
	_poiProvider->add(glm::vec3(1.0));
	_poiProvider->add(glm::vec3(2.0));
	_poiProvider->add(glm::vec3(3.0));
	EXPECT_EQ(3u, _poiProvider->count());
	_timeProvider->setTickTime(60 * 1000UL);
	_poiProvider->update(0UL);
	EXPECT_GE(_poiProvider->count(), 1u) << "We should have at least one poi left";
}

TEST_F(PoiProviderTest, testExpireWithProperPos) {
	const int max = 3;
	for (int i = 0; i < max; ++i) {
		_timeProvider->setTickTime(i * 60 * 1000UL);
		_poiProvider->add(glm::vec3(static_cast<float>(i)), Type::GENERIC);
	}
	EXPECT_EQ(3u, _poiProvider->count());
	// this should expire everything
	_timeProvider->setTickTime(60000000UL);
	_poiProvider->update(0UL);
	EXPECT_EQ(1u, _poiProvider->count()) << "We should have at least one poi left - but every other should be expired";
	const poi::PoiResult& result = _poiProvider->query();
	EXPECT_TRUE(result.valid);
	EXPECT_EQ(glm::vec3(static_cast<float>(max - 1)),  result.pos) << max - 1 << " versus " << glm::to_string(result.pos);
}

TEST_F(PoiProviderTest, testQueryType) {
	_poiProvider->add(glm::vec3(0.0), Type::GENERIC);
	_poiProvider->add(glm::vec3(1.0), Type::GENERIC);
	_poiProvider->add(glm::vec3(2.0), Type::FIGHT);
	_poiProvider->add(glm::vec3(3.0), Type::QUEST);
	_poiProvider->add(glm::vec3(4.0), Type::QUEST);
	_poiProvider->add(glm::vec3(5.0), Type::QUEST);
	_poiProvider->add(glm::vec3(6.0), Type::QUEST);
	const PoiResult& result = _poiProvider->query(Type::FIGHT);
	EXPECT_TRUE(result.valid);
	EXPECT_EQ(glm::vec3(2.0), result.pos);
}

TEST_F(PoiProviderTest, testNoExpire) {
	_poiProvider->add(glm::vec3(1.0));
	_poiProvider->add(glm::vec3(2.0));
	_poiProvider->add(glm::vec3(3.0));
	EXPECT_EQ(3u, _poiProvider->count());
	_timeProvider->setTickTime(30 * 1000UL);
	_poiProvider->update(0UL);
	EXPECT_EQ(3u, _poiProvider->count()) << "We should still have all three left";
}

}
