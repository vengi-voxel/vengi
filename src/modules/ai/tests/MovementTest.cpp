/**
 * @file
 */

#include "MovementTest.h"
#include "movement/SelectionSeek.h"
#include "movement/SelectionFlee.h"
#include "movement/GroupFlee.h"
#include "movement/GroupSeek.h"
#include "movement/Steering.h"
#include "movement/TargetFlee.h"
#include "movement/TargetSeek.h"
#include "movement/Wander.h"
#include "movement/WeightedSteering.h"
#include <glm/gtc/constants.hpp>

class MovementTest: public TestSuite {
protected:
	const float _speed = 100.0f;
};

TEST_F(MovementTest, testFlee) {
	ai::movement::TargetFlee flee("0:0:0");
	const ai::AIPtr& ai = std::make_shared<ai::AI>(ai::TreeNodePtr());
	const ai::ICharacterPtr& entity = std::make_shared<ai::ICharacter>(1);
	ai->setCharacter(entity);

	// flee to the left (negative x)
	entity->setPosition(glm::vec3(-1, 0, 0));
	const ai::MoveVector& mvLeft = flee.execute(ai, _speed);
	EXPECT_EQ(glm::vec3(-_speed, 0.0f, 0.0f), mvLeft.getVector()) << ::testing::PrintToString(mvLeft.getVector());
	EXPECT_FLOAT_EQ(glm::pi<float>(), mvLeft.getOrientation(1.0f));

	// flee to the right (positive x)
	entity->setPosition(glm::vec3(1, 0, 0));
	const ai::MoveVector& mvRight = flee.execute(ai, _speed);
	EXPECT_EQ(glm::vec3(_speed, 0.0f, 0.0f), mvRight.getVector()) << ::testing::PrintToString(mvRight.getVector());
	EXPECT_FLOAT_EQ(0.0f, mvRight.getOrientation(1.0f));

	// flee into positive z
	entity->setPosition(glm::vec3(0, 0, 1));
	const ai::MoveVector& mvPosZ = flee.execute(ai, _speed);
	EXPECT_EQ(glm::vec3(0.0f, 0.0f, _speed), mvPosZ.getVector()) << ::testing::PrintToString(mvPosZ.getVector());
	EXPECT_FLOAT_EQ(glm::two_pi<float>(), mvPosZ.getOrientation(1.0f));

	// flee into negative z
	entity->setPosition(glm::vec3(0, 0, -1));
	const ai::MoveVector& mvNegZ = flee.execute(ai, _speed);
	EXPECT_EQ(glm::vec3(0.0f, 0.0f, -_speed), mvNegZ.getVector()) << ::testing::PrintToString(mvNegZ.getVector());
	EXPECT_FLOAT_EQ(glm::two_pi<float>() + glm::pi<float>(), mvNegZ.getOrientation(1.0f));
}

TEST_F(MovementTest, testWanderWithoutOrientationChange) {
	ai::movement::Wander wander("0.0");
	const ai::AIPtr& ai = std::make_shared<ai::AI>(ai::TreeNodePtr());
	const ai::ICharacterPtr& entity = std::make_shared<ai::ICharacter>(1);
	ai->setCharacter(entity);

	const double eps = 0.00001;

	// moving to the right (positive x)
	entity->setOrientation(0.0f);
	const ai::MoveVector& mvRight = wander.execute(ai, _speed);
	EXPECT_FLOAT_EQ(_speed, mvRight.getVector().x) << ::testing::PrintToString(mvRight.getVector());
	EXPECT_FLOAT_EQ(0.0f, mvRight.getVector().y) << ::testing::PrintToString(mvRight.getVector());
	EXPECT_FLOAT_EQ(0.0f, mvRight.getVector().z) << ::testing::PrintToString(mvRight.getVector());
	EXPECT_EQ(0.0f, mvRight.getOrientation(1.0f));

	// moving to the left (negative x)
	entity->setOrientation(glm::pi<float>());
	const ai::MoveVector& mvLeft = wander.execute(ai, _speed);
	EXPECT_FLOAT_EQ(-_speed, mvLeft.getVector().x) << ::testing::PrintToString(mvLeft.getVector());
	EXPECT_FLOAT_EQ(0.0f, mvLeft.getVector().y) << ::testing::PrintToString(mvLeft.getVector());
	EXPECT_NEAR(0.0f, mvLeft.getVector().z, eps) << ::testing::PrintToString(mvLeft.getVector());
	EXPECT_EQ(0.0f, mvLeft.getOrientation(1.0f));

	// moving into positive z
	entity->setOrientation(glm::half_pi<float>());
	const ai::MoveVector& mvPosZ = wander.execute(ai, _speed);
	EXPECT_FLOAT_EQ(_speed, mvPosZ.getVector().z) << ::testing::PrintToString(mvPosZ.getVector());
	EXPECT_NEAR(0.0f, mvPosZ.getVector().x, eps) << ::testing::PrintToString(mvPosZ.getVector());
	EXPECT_FLOAT_EQ(0.0f, mvPosZ.getVector().y) << ::testing::PrintToString(mvPosZ.getVector());
	EXPECT_EQ(0.0f, mvPosZ.getOrientation(1.0f));

	// moving negative z
	entity->setOrientation(glm::three_over_two_pi<float>());
	const ai::MoveVector& mvNegZ = wander.execute(ai, _speed);
	EXPECT_FLOAT_EQ(-_speed, mvNegZ.getVector().z) << ::testing::PrintToString(mvNegZ.getVector());
	EXPECT_NEAR(0.0f, mvNegZ.getVector().x, eps) << ::testing::PrintToString(mvNegZ.getVector());
	EXPECT_FLOAT_EQ(0.0f, mvNegZ.getVector().y) << ::testing::PrintToString(mvNegZ.getVector());
	EXPECT_EQ(0.0f, mvNegZ.getOrientation(1.0f));
}

TEST_F(MovementTest, testWeightedSteering) {
	ai::randomSeed(0);

	ai::Zone zone("movementTest");
	const ai::AIPtr& ai = std::make_shared<ai::AI>(ai::TreeNodePtr());
	const ai::ICharacterPtr& entity = std::make_shared<ai::ICharacter>(1);
	ai->setCharacter(entity);
	entity->setOrientation(0.0f);
	entity->setPosition(glm::vec3(0, 0, 0));
	zone.addAI(ai);

	const ai::SteeringPtr& flee = std::make_shared<ai::movement::TargetFlee>("1:0:0");
	const ai::SteeringPtr& wander = std::make_shared<ai::movement::Wander>("0");

	ai::movement::WeightedSteerings s;
	s.push_back(ai::movement::WeightedData(flee, 0.8f));
	s.push_back(ai::movement::WeightedData(wander, 0.2f));

	ai::movement::WeightedSteering w(s);
	const ai::MoveVector& mv = w.execute(ai, 100.0f);
	EXPECT_NEAR((glm::pi<float>() * 0.8f) + (0.0f * 0.2f), mv.getOrientation(1.0f), 0.00001f);
	const glm::vec3 result = glm::vec3(-_speed, 0.0f, 0.0f) * 0.8f + glm::vec3(_speed, 0.0f, 0.0f) * 0.2f;
	EXPECT_EQ(result, mv.getVector());
}
