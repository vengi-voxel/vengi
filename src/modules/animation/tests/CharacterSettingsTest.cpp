/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "animation/AnimationSettings.h"
#include "animation/chr/CharacterSkeletonAttribute.h"

namespace animation {

const char *TestSetters = R"(
function init()
  settings.setBasePath("testrace", "testgender")
  settings.setMeshTypes("type1", "type2")
  settings.setPath("type1", "name1")
  settings.setPath("type2", "name2")
  local attributes = {
    scaler = 42.0,
    headScale = 1337.0,
    neckHeight = 815.0,
    neckForward = 4311.0,
    neckRight = 3.14,
    toolForward = 1.0,
    toolRight = -1.0,
    shoulderScale = 100.0,
    headHeight = 101.0,
    chestHeight = 102.0,
    beltHeight = 103.0,
    pantsHeight = 104.0,
    invisibleLegHeight = 105.0,
    footHeight = 106.0,
    origin = 108.0,
    hipOffset = 109.0,
    footRight = -3.2
  }
  return attributes
end
)";

class CharacterSettingsTest: public core::AbstractTest {
};

TEST_F(CharacterSettingsTest, testLUA) {
	AnimationSettings settings;
	CharacterSkeletonAttribute skeletonAttr;
	ASSERT_TRUE(loadAnimationSettings(TestSetters, settings, &skeletonAttr, ChrSkeletonAttributeMetaArray))
		<< "Failed to initialize the character settings";

	ASSERT_EQ(2u, settings.types().size());

	EXPECT_EQ(0, settings.getMeshTypeIdxForName("type1"));
	EXPECT_EQ(1, settings.getMeshTypeIdxForName("type2"));

	EXPECT_EQ("type1/name1", settings.path(0));
	EXPECT_EQ("type2/name2", settings.path(1));

	EXPECT_FLOAT_EQ(  42.0f,  skeletonAttr.scaler);
	EXPECT_FLOAT_EQ(1337.0f,  skeletonAttr.headScale);
	EXPECT_FLOAT_EQ( 815.0f,  skeletonAttr.neckHeight);
	EXPECT_FLOAT_EQ(4311.0f,  skeletonAttr.neckForward);
	EXPECT_FLOAT_EQ(   3.14f, skeletonAttr.neckRight);
	EXPECT_FLOAT_EQ(   1.0f,  skeletonAttr.toolForward);
	EXPECT_FLOAT_EQ(  -1.0f,  skeletonAttr.toolRight);
	EXPECT_FLOAT_EQ(  -3.2f,  skeletonAttr.footRight);

	EXPECT_FLOAT_EQ( 100.0f,  skeletonAttr.shoulderScale);
	EXPECT_FLOAT_EQ( 101.0f,  skeletonAttr.headHeight);
	EXPECT_FLOAT_EQ( 102.0f,  skeletonAttr.chestHeight);
	EXPECT_FLOAT_EQ( 103.0f,  skeletonAttr.beltHeight);
	EXPECT_FLOAT_EQ( 104.0f,  skeletonAttr.pantsHeight);
	EXPECT_FLOAT_EQ( 105.0f,  skeletonAttr.invisibleLegHeight);
	EXPECT_FLOAT_EQ( 106.0f,  skeletonAttr.footHeight);
	EXPECT_FLOAT_EQ( 108.0f,  skeletonAttr.origin);
	EXPECT_FLOAT_EQ( 109.0f,  skeletonAttr.hipOffset);
}

}
