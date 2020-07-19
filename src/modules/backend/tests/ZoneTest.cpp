/**
 * @file
 */

#include "TestShared.h"
#include "backend/entity/ai/tree/PrioritySelector.h"
#include "backend/entity/ai/zone/Zone.h"

namespace backend {

class ZoneTest: public TestSuite {
};

TEST_F(ZoneTest, testChanges) {
	Zone zone("test1");
	TreeNodePtr root = std::make_shared<PrioritySelector>("test", "", True::get());
	ICharacterPtr character = std::make_shared<TestEntity>(1);
	AIPtr ai = std::make_shared<AI>(root);
	ai->setCharacter(character);

	ICharacterPtr character2 = std::make_shared<TestEntity>(2);
	AIPtr ai2 = std::make_shared<AI>(root);
	ai2->setCharacter(character2);

	ASSERT_TRUE(zone.addAI(ai)) << "Could not add ai to the zone";
	zone.setDebug(true);
	zone.update(1);
	ASSERT_TRUE(ai->isDebuggingActive()) << "Debug is not active for the entity";
	ASSERT_TRUE(zone.addAI(ai2)) << "Could not add ai to the zone";
	zone.update(1);
	ASSERT_TRUE(ai2->isDebuggingActive()) << "Debug is not active for newly added entity";
	zone.setDebug(false);
	zone.update(1);
	ASSERT_FALSE(ai->isDebuggingActive()) << "Debug is still active for entity";
	ASSERT_FALSE(ai2->isDebuggingActive()) << "Debug is still active for newly added entity";
	ASSERT_TRUE(zone.removeAI(1)) << "Could not remove ai from zone";
	ASSERT_TRUE(zone.removeAI(2)) << "Could not remove ai from zone";
}

TEST_F(ZoneTest, testAdd100) {
	Zone zone("test1");
	TreeNodePtr root = std::make_shared<PrioritySelector>("test", "", True::get());
	const int n = 100;
	for (int i = 0; i < n; ++i) {
		ICharacterPtr character = std::make_shared<TestEntity>(i);
		AIPtr ai = std::make_shared<AI>(root);
		ai->setCharacter(character);
		ASSERT_TRUE(zone.addAI(ai)) << "Could not add ai to the zone";
	}
	zone.update(0l);
	ASSERT_EQ(n, (int)zone.size());
}

}
