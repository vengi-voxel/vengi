/**
 * @file
 */

#include "TestShared.h"
#include "backend/entity/ai/movement/SelectionSeek.h"
#include "backend/entity/ai/movement/SelectionFlee.h"
#include "backend/entity/ai/movement/GroupFlee.h"
#include "backend/entity/ai/movement/GroupSeek.h"
#include "backend/entity/ai/movement/Steering.h"
#include "backend/entity/ai/movement/TargetFlee.h"
#include "backend/entity/ai/movement/TargetSeek.h"
#include "backend/entity/ai/movement/Wander.h"
#include "backend/entity/ai/movement/WeightedSteering.h"
#include "backend/entity/ai/zone/Zone.h"
#include "backend/entity/ai/common/Random.h"
#include <glm/gtc/constants.hpp>

namespace backend {

class MovementTest: public TestSuite {
protected:
	const float _speed = 100.0f;
};

TEST_F(MovementTest, testFlee) {
	movement::TargetFlee flee("0:0:0");
	const AIPtr& ai = std::make_shared<AI>(TreeNodePtr());
	const ICharacterPtr& entity = core::make_shared<ICharacter>(1);
	ai->setCharacter(entity);

	// flee to the left (negative x)
	entity->setPosition(glm::vec3(-1, 0, 0));
	const MoveVector& mvLeft = flee.execute(ai, _speed);
	EXPECT_EQ(glm::vec3(-_speed, 0.0f, 0.0f), mvLeft.getVector()) << ::testing::PrintToString(mvLeft.getVector());
	EXPECT_FLOAT_EQ(glm::pi<float>(), mvLeft.getOrientation(1.0f));

	// flee to the right (positive x)
	entity->setPosition(glm::vec3(1, 0, 0));
	const MoveVector& mvRight = flee.execute(ai, _speed);
	EXPECT_EQ(glm::vec3(_speed, 0.0f, 0.0f), mvRight.getVector()) << ::testing::PrintToString(mvRight.getVector());
	EXPECT_FLOAT_EQ(0.0f, mvRight.getOrientation(1.0f));

	// flee into positive z
	entity->setPosition(glm::vec3(0, 0, 1));
	const MoveVector& mvPosZ = flee.execute(ai, _speed);
	EXPECT_EQ(glm::vec3(0.0f, 0.0f, _speed), mvPosZ.getVector()) << ::testing::PrintToString(mvPosZ.getVector());
	EXPECT_FLOAT_EQ(glm::half_pi<float>(), mvPosZ.getOrientation(1.0f));

	// flee into negative z
	entity->setPosition(glm::vec3(0, 0, -1));
	const MoveVector& mvNegZ = flee.execute(ai, _speed);
	EXPECT_EQ(glm::vec3(0.0f, 0.0f, -_speed), mvNegZ.getVector()) << ::testing::PrintToString(mvNegZ.getVector());
	EXPECT_FLOAT_EQ(glm::half_pi<float>() + glm::pi<float>(), mvNegZ.getOrientation(1.0f));
}

TEST_F(MovementTest, testWanderWithoutOrientationChange) {
	backend::movement::Wander wander("0.0");
	const AIPtr& ai = std::make_shared<AI>(TreeNodePtr());
	const ICharacterPtr& entity = core::make_shared<ICharacter>(1);
	ai->setCharacter(entity);

	const double eps = 0.00001;

	// moving to the right (positive x)
	entity->setOrientation(0.0f);
	const MoveVector& mvRight = wander.execute(ai, _speed);
	EXPECT_FLOAT_EQ(_speed, mvRight.getVector().x) << ::testing::PrintToString(mvRight.getVector());
	EXPECT_FLOAT_EQ(0.0f, mvRight.getVector().y) << ::testing::PrintToString(mvRight.getVector());
	EXPECT_FLOAT_EQ(0.0f, mvRight.getVector().z) << ::testing::PrintToString(mvRight.getVector());
	EXPECT_EQ(0.0f, mvRight.getOrientation(1.0f));

	// moving to the left (negative x)
	entity->setOrientation(glm::pi<float>());
	const MoveVector& mvLeft = wander.execute(ai, _speed);
	EXPECT_FLOAT_EQ(-_speed, mvLeft.getVector().x) << ::testing::PrintToString(mvLeft.getVector());
	EXPECT_FLOAT_EQ(0.0f, mvLeft.getVector().y) << ::testing::PrintToString(mvLeft.getVector());
	EXPECT_NEAR(0.0f, mvLeft.getVector().z, eps) << ::testing::PrintToString(mvLeft.getVector());
	EXPECT_EQ(0.0f, mvLeft.getOrientation(1.0f));

	// moving into positive z
	entity->setOrientation(glm::half_pi<float>());
	const MoveVector& mvPosZ = wander.execute(ai, _speed);
	EXPECT_FLOAT_EQ(_speed, mvPosZ.getVector().z) << ::testing::PrintToString(mvPosZ.getVector());
	EXPECT_NEAR(0.0f, mvPosZ.getVector().x, eps) << ::testing::PrintToString(mvPosZ.getVector());
	EXPECT_FLOAT_EQ(0.0f, mvPosZ.getVector().y) << ::testing::PrintToString(mvPosZ.getVector());
	EXPECT_EQ(0.0f, mvPosZ.getOrientation(1.0f));

	// moving negative z
	entity->setOrientation(glm::three_over_two_pi<float>());
	const MoveVector& mvNegZ = wander.execute(ai, _speed);
	EXPECT_FLOAT_EQ(-_speed, mvNegZ.getVector().z) << ::testing::PrintToString(mvNegZ.getVector());
	EXPECT_NEAR(0.0f, mvNegZ.getVector().x, eps) << ::testing::PrintToString(mvNegZ.getVector());
	EXPECT_FLOAT_EQ(0.0f, mvNegZ.getVector().y) << ::testing::PrintToString(mvNegZ.getVector());
	EXPECT_EQ(0.0f, mvNegZ.getOrientation(1.0f));
}

TEST_F(MovementTest, testWeightedSteering) {
	backend::randomSeed(0);

	Zone zone("movementTest");
	const AIPtr& ai = std::make_shared<AI>(TreeNodePtr());
	const ICharacterPtr& entity = core::make_shared<ICharacter>(1);
	ai->setCharacter(entity);
	entity->setOrientation(0.0f);
	entity->setPosition(glm::vec3(0, 0, 0));
	zone.addAI(ai);

	const backend::SteeringPtr& flee = std::make_shared<backend::movement::TargetFlee>("1:0:0");
	const backend::SteeringPtr& wander = std::make_shared<backend::movement::Wander>("0");

	backend::movement::WeightedSteerings s;
	s.push_back(backend::movement::WeightedData(flee, 0.8f));
	s.push_back(backend::movement::WeightedData(wander, 0.2f));

	backend::movement::WeightedSteering w(s);
	const MoveVector& mv = w.execute(ai, 100.0f);
	EXPECT_NEAR((glm::pi<float>() * 0.8f) + (0.0f * 0.2f), mv.getOrientation(1.0f), 0.00001f);
	const glm::vec3 result = glm::vec3(-_speed, 0.0f, 0.0f) * 0.8f + glm::vec3(_speed, 0.0f, 0.0f) * 0.2f;
	EXPECT_EQ(result, mv.getVector());
}

}
