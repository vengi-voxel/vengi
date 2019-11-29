/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "animation/chr/CharacterSettings.h"

namespace animation {

const char *TestSetters = R"(
function init()
  chr.setBasePath("testrace", "testgender")
  chr.setPath("head", "heads/test")
  chr.setPath("belt", "belts/test")
  chr.setPath("chest", "chests/test")
  chr.setPath("pants", "pants/test")
  chr.setPath("hand", "hands/test")
  chr.setPath("foot", "feet/test")
  chr.setScaler(42.0)
  chr.setHeadScale(1337.0)
  chr.setNeckHeight(815.0)
  chr.setNeckForward(4311.0)
  chr.setNeckRight(3.14)
  chr.setToolForward(1.0)
  chr.setToolRight(-1.0)
  chr.setShoulderScale(100.0)
  chr.setHeadHeight(101.0)
  chr.setChestHeight(102.0)
  chr.setBeltHeight(103.0)
  chr.setPantsHeight(104.0)
  chr.setInvisibleLegHeight(105.0)
  chr.setFootHeight(106.0)
  chr.setOrigin(108.0)
  chr.setHipOffset(109.0)
  chr.setFootRight(-3.2)
end
)";

class CharacterSettingsTest: public core::AbstractTest {
};

TEST_F(CharacterSettingsTest, testLUA) {
	CharacterSettings settings;
	EXPECT_TRUE(loadCharacterSettings(TestSetters, settings))
		<< "Failed to initialize the character settings";

	EXPECT_EQ("heads/test", settings.path("head"));
	EXPECT_EQ("belts/test", settings.path("belt"));
	EXPECT_EQ("chests/test", settings.path("chest"));
	EXPECT_EQ("pants/test", settings.path("pants"));
	EXPECT_EQ("hands/test", settings.path("hand"));
	EXPECT_EQ("feet/test", settings.path("foot"));

	EXPECT_FLOAT_EQ(  42.0f,  settings.skeletonAttr.scaler);
	EXPECT_FLOAT_EQ(1337.0f,  settings.skeletonAttr.headScale);
	EXPECT_FLOAT_EQ( 815.0f,  settings.skeletonAttr.neckHeight);
	EXPECT_FLOAT_EQ(4311.0f,  settings.skeletonAttr.neckForward);
	EXPECT_FLOAT_EQ(   3.14f, settings.skeletonAttr.neckRight);
	EXPECT_FLOAT_EQ(   1.0f,  settings.skeletonAttr.toolForward);
	EXPECT_FLOAT_EQ(  -1.0f,  settings.skeletonAttr.toolRight);
	EXPECT_FLOAT_EQ(  -3.2f,  settings.skeletonAttr.footRight);

	EXPECT_FLOAT_EQ( 100.0f,  settings.skeletonAttr.shoulderScale);
	EXPECT_FLOAT_EQ( 101.0f,  settings.skeletonAttr.headHeight);
	EXPECT_FLOAT_EQ( 102.0f,  settings.skeletonAttr.chestHeight);
	EXPECT_FLOAT_EQ( 103.0f,  settings.skeletonAttr.beltHeight);
	EXPECT_FLOAT_EQ( 104.0f,  settings.skeletonAttr.pantsHeight);
	EXPECT_FLOAT_EQ( 105.0f,  settings.skeletonAttr.invisibleLegHeight);
	EXPECT_FLOAT_EQ( 106.0f,  settings.skeletonAttr.footHeight);
	EXPECT_FLOAT_EQ( 108.0f,  settings.skeletonAttr.origin);
	EXPECT_FLOAT_EQ( 109.0f,  settings.skeletonAttr.hipOffset);
}

}
