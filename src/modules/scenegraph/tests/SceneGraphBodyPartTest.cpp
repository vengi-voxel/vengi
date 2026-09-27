/**
 * @file
 */

#include "scenegraph/SceneGraphBodyPart.h"
#include "app/tests/AbstractTest.h"

namespace scenegraph {

class SceneGraphBodyPartTest : public app::AbstractTest {};

TEST_F(SceneGraphBodyPartTest, testIdentifyExamples) {
	EXPECT_STREQ("torso", identifyBodyPart("belt").c_str());
	EXPECT_STREQ("torso", identifyBodyPart("K_Torso").c_str());
	EXPECT_STREQ("head", identifyBodyPart("head").c_str());
	EXPECT_STREQ("left_arm", identifyBodyPart("arm_left").c_str());
	EXPECT_STREQ("left_arm", identifyBodyPart("K_Arm_Left").c_str());
	EXPECT_STREQ("right_upper_leg", identifyBodyPart("right_leg").c_str());
	EXPECT_STREQ("right_upper_leg", identifyBodyPart("K_Leg_Right_u").c_str());
	EXPECT_STREQ("left_lower_leg", identifyBodyPart("K_Leg_Left_l").c_str());
	EXPECT_STREQ("right_shoulder", identifyBodyPart("shoulder_r").c_str());
	EXPECT_STREQ("left_hand", identifyBodyPart("hand_left").c_str());
	EXPECT_STREQ("cover", identifyBodyPart("cape").c_str());
	EXPECT_TRUE(identifyBodyPart("layer_a").empty());
}

TEST_F(SceneGraphBodyPartTest, testDetectSide) {
	EXPECT_STREQ("left", detectBodyPartSide("arm_left").c_str());
	EXPECT_STREQ("right", detectBodyPartSide("shoulder_r").c_str());
	EXPECT_STREQ("left", detectBodyPartSide("l_arm").c_str());
	EXPECT_TRUE(detectBodyPartSide("torso").empty());
}

} // namespace scenegraph
