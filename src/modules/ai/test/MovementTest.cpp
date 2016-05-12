#include "MovementTest.h"
#include "core/GLM.h"

class MovementTest: public TestSuite {
};

TEST_F(MovementTest, testFlee) {
	ai::movement::TargetFlee flee("0:0:0");
	ai::AIPtr ai(new ai::AI(ai::TreeNodePtr()));
	ai::ICharacterPtr entity(new ai::ICharacter(1));
	ai->setCharacter(entity);

	// flee to the left
	entity->setPosition(glm::vec3(-1, 0, 0));
	const ai::MoveVector& mvLeft = flee.execute(ai, 100);
	ASSERT_EQ(glm::vec3(-100.0f, 0.0f, 0.0f), mvLeft.getVector());
	ASSERT_FLOAT_EQ(glm::pi<float>(), mvLeft.getOrientation(1.0f));

	// flee to the right
	entity->setPosition(glm::vec3(1, 0, 0));
	const ai::MoveVector& mvRight = flee.execute(ai, 100);
	ASSERT_EQ(glm::vec3(100.0f, 0.0f, 0.0f), mvRight.getVector());
	ASSERT_FLOAT_EQ(0.0f, mvRight.getOrientation(1.0f));

	// flee into positive z
	entity->setPosition(glm::vec3(0, 0, 1));
	const ai::MoveVector& mvPosZ = flee.execute(ai, 100);
	ASSERT_EQ(glm::vec3(0.0f, 0.0f, 100.0f), mvPosZ.getVector());
	ASSERT_FLOAT_EQ(glm::half_pi<float>(), mvPosZ.getOrientation(1.0f));

	// flee into negative z
	entity->setPosition(glm::vec3(0, 0, -1));
	const ai::MoveVector& mvNegZ = flee.execute(ai, 100);
	ASSERT_EQ(glm::vec3(0.0f, 0.0f, -100.0f), mvNegZ.getVector());
	ASSERT_FLOAT_EQ(glm::half_pi<float>() + glm::pi<float>(), mvNegZ.getOrientation(1.0f));
}

TEST_F(MovementTest, testWanderWithoutOrientationChange) {
	ai::movement::Wander wander("0.0");
	ai::AIPtr ai(new ai::AI(ai::TreeNodePtr()));
	ai::ICharacterPtr entity(new ai::ICharacter(1));
	ai->setCharacter(entity);

	// moving to the right
	entity->setOrientation(0.0f);
	const ai::MoveVector& mvRight = wander.execute(ai, 100);
	ASSERT_EQ(glm::vec3(100.0f, 0.0f, 0.0f), mvRight.getVector());
	ASSERT_EQ(0.0f, mvRight.getOrientation(1.0f));

	// moving to the left
	entity->setOrientation(glm::pi<float>());
	const ai::MoveVector& mvLeft = wander.execute(ai, 100);
	ASSERT_EQ(glm::vec3(-100.0f, 0.0f, 0.0f), mvLeft.getVector());
	ASSERT_EQ(0.0f, mvLeft.getOrientation(1.0f));

	// moving into positive z
	entity->setOrientation(glm::half_pi<float>());
	const ai::MoveVector& mvPosZ = wander.execute(ai, 100);
	ASSERT_EQ(glm::vec3(0.0f, 0.0f, 100.0f), mvPosZ.getVector());
	ASSERT_EQ(0.0f, mvPosZ.getOrientation(1.0f));

	// moving negative z
	entity->setOrientation(glm::half_pi<float>() + glm::pi<float>());
	const ai::MoveVector& mvNegZ = wander.execute(ai, 100);
	ASSERT_EQ(glm::vec3(0.0f, 0.0f, -100.0f), mvNegZ.getVector());
	ASSERT_EQ(0.0f, mvNegZ.getOrientation(1.0f));
}

TEST_F(MovementTest, testWeightedSteering) {
	ai::randomSeed(0);

	ai::Zone zone("movementTest");
	ai::AIPtr ai(new ai::AI(ai::TreeNodePtr()));
	ai::ICharacterPtr entity(new ai::ICharacter(1));
	ai->setCharacter(entity);
	entity->setOrientation(0.0f);
	entity->setPosition(glm::vec3(0, 0, 0));
	zone.addAI(ai);

	ai::SteeringPtr flee(new ai::movement::TargetFlee("1:0:0"));
	ai::SteeringPtr wander(new ai::movement::Wander("0"));

	ai::movement::WeightedSteerings s;
	s.push_back(ai::movement::WeightedData(flee, 0.8f));
	s.push_back(ai::movement::WeightedData(wander, 0.2f));

	ai::movement::WeightedSteering w(s);
	const ai::MoveVector& mv = w.execute(ai, 100.0f);
	ASSERT_NEAR((M_PI * 0.8f) + (0.0f * 0.2f), mv.getOrientation(1.0f), 0.00001f);
	const glm::vec3 result = glm::vec3(-100.0f, 0.0f, 0.0f) * 0.8f + glm::vec3(100.0f, 0.0f, 0.0f) * 0.2f;
	ASSERT_EQ(result, mv.getVector());
}
