/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "poi/PoiProvider.h"
#include "voxelworld/WorldMgr.h"

namespace poi {

class PoiProviderTest: public core::AbstractTest {
public:
	core::TimeProviderPtr _timeProvider;
	void SetUp() override {
		core::AbstractTest::SetUp();
		_timeProvider = std::make_shared<core::TimeProvider>();
	}
};

TEST_F(PoiProviderTest, testUpdate) {
	PoiProvider poiProvider(_timeProvider);
	poiProvider.add(glm::vec3(1.0));
	const poi::PoiResult& result = poiProvider.query();
	EXPECT_TRUE(result.valid);
	EXPECT_EQ(glm::vec3(1.0), result.pos);
}

TEST_F(PoiProviderTest, testExpire) {
	PoiProvider poiProvider(_timeProvider);
	poiProvider.add(glm::vec3(1.0));
	poiProvider.add(glm::vec3(2.0));
	poiProvider.add(glm::vec3(3.0));
	EXPECT_EQ(3u, poiProvider.count());
	_timeProvider->setTickTime(60 * 1000UL);
	poiProvider.update(0UL);
	EXPECT_GE(poiProvider.count(), 1u) << "We should have at least one poi left";
}

TEST_F(PoiProviderTest, testExpireWithProperPos) {
	PoiProvider poiProvider(_timeProvider);
	const int max = 3;
	for (int i = 0; i < max; ++i) {
		_timeProvider->setTickTime(i * 60 * 1000UL);
		poiProvider.add(glm::vec3(static_cast<float>(i)), Type::GENERIC);
	}
	EXPECT_EQ(3u, poiProvider.count());
	// this should expire everything
	_timeProvider->setTickTime(60000000UL);
	poiProvider.update(0UL);
	EXPECT_EQ(1u, poiProvider.count()) << "We should have at least one poi left - but every other should be expired";
	const poi::PoiResult& result = poiProvider.query();
	EXPECT_TRUE(result.valid);
	EXPECT_EQ(glm::vec3(static_cast<float>(max - 1)),  result.pos) << max - 1 << " versus " << glm::to_string(result.pos);
}

TEST_F(PoiProviderTest, testQueryType) {
	PoiProvider poiProvider(_timeProvider);
	poiProvider.add(glm::vec3(0.0), Type::GENERIC);
	poiProvider.add(glm::vec3(1.0), Type::GENERIC);
	poiProvider.add(glm::vec3(2.0), Type::FIGHT);
	poiProvider.add(glm::vec3(3.0), Type::QUEST);
	poiProvider.add(glm::vec3(4.0), Type::QUEST);
	poiProvider.add(glm::vec3(5.0), Type::QUEST);
	poiProvider.add(glm::vec3(6.0), Type::QUEST);
	const PoiResult& result = poiProvider.query(Type::FIGHT);
	EXPECT_TRUE(result.valid);
	EXPECT_EQ(glm::vec3(2.0), result.pos);
}

TEST_F(PoiProviderTest, testNoExpire) {
	PoiProvider poiProvider(_timeProvider);
	poiProvider.add(glm::vec3(1.0));
	poiProvider.add(glm::vec3(2.0));
	poiProvider.add(glm::vec3(3.0));
	EXPECT_EQ(3u, poiProvider.count());
	_timeProvider->setTickTime(30 * 1000UL);
	poiProvider.update(0UL);
	EXPECT_EQ(3u, poiProvider.count()) << "We should still have all three left";
}

}
