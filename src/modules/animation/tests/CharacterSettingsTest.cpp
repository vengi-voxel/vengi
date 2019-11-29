/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "animation/chr/CharacterSettings.h"

namespace animation {

const char *TestSetters = R"(
function init()
  settings.setBasePath("testrace", "testgender")
  settings.setMeshTypes("head", "chest", "belt", "pants", "hand", "foot", "shoulder", "glider")
  settings.setPath("head", "head")
  settings.setPath("belt", "belt")
  settings.setPath("chest", "chest")
  settings.setPath("pants", "pants")
  settings.setPath("hand", "hand")
  settings.setPath("foot", "foot")
  settings.setPath("glider", "glider")
  skeleton.setScaler(42.0)
  skeleton.setHeadScale(1337.0)
  skeleton.setNeckHeight(815.0)
  skeleton.setNeckForward(4311.0)
  skeleton.setNeckRight(3.14)
  skeleton.setToolForward(1.0)
  skeleton.setToolRight(-1.0)
  skeleton.setShoulderScale(100.0)
  skeleton.setHeadHeight(101.0)
  skeleton.setChestHeight(102.0)
  skeleton.setBeltHeight(103.0)
  skeleton.setPantsHeight(104.0)
  skeleton.setInvisibleLegHeight(105.0)
  skeleton.setFootHeight(106.0)
  skeleton.setOrigin(108.0)
  skeleton.setHipOffset(109.0)
  skeleton.setFootRight(-3.2)
end
)";

class CharacterSettingsTest: public core::AbstractTest {
};

TEST_F(CharacterSettingsTest, testLUA) {
	CharacterSettings settings;
	EXPECT_TRUE(loadCharacterSettings(TestSetters, settings))
		<< "Failed to initialize the character settings";

	EXPECT_EQ(8u, settings.types().size());
	for (size_t i = 0; i < settings.types().size(); ++i) {
		EXPECT_EQ(settings.types()[i], settings.path(i));
	}

	const CharacterSkeletonAttribute& skeletonAttr = settings.skeletonAttr;
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
