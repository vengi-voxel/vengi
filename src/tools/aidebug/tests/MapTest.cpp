/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "../Map.h"

class MapTest: public app::AbstractTest {
private:
	using Super = app::AbstractTest;
public:
	Map _map;

	void SetUp() override {
		Super::SetUp();
		_map = Map();
		_map.setMinsMaxs(glm::ivec2(0), glm::ivec2(200, 100));
	}
};

TEST_F(MapTest, testConvertCoordinatesNoScroll) {
	EXPECT_EQ(glm::ivec2(0), _map.entPosToMap(0.0f, 0.0f));
	EXPECT_EQ(glm::ivec2(500), _map.entPosToMap(500.0f, 500.0f));
}

TEST_F(MapTest, testConvertCoordinatesNoScrollNeeded) {
	_map.centerAtEntPos(100.0f, 50.0f);
	EXPECT_EQ(glm::ivec2(0), _map.entPosToMap(0.0f, 0.0f));
}

TEST_F(MapTest, testConvertCoordinatesScrolled) {
	_map.centerAtEntPos(200.0f, 100.0f);
	EXPECT_EQ(glm::ivec2(-100, -50), _map.entPosToMap(0.0f, 0.0f));
}
