/**
 * @file
 */

#include "TestShared.h"

class AggroTest: public TestSuite {
public:
	void doMassTest(int max) {
		ai::AggroMgr mgr(max);
		for (int i = 1; i <= max; ++i) {
			const ai::CharacterId id = i;
			ai::ICharacterPtr e(new TestEntity(id));
			ai::AIPtr ai(new ai::AI(ai::TreeNodePtr()));
			ai->setCharacter(e);
			ai::Entry* entry = mgr.addAggro(id, i);
			entry->setReduceByValue(i);
		}
		const ai::EntryPtr& entry = mgr.getHighestEntry();
		ASSERT_TRUE(entry) << "Highest entry not set but aggro was added";
		ASSERT_EQ(max, entry->getCharacterId())<< "Highest entry not what it should be. " << printAggroList(mgr);
		mgr.update(1000);
		ASSERT_EQ(0u, mgr.count());
	}
};

TEST_F(AggroTest, testAggroMgr) {
	ai::AggroMgr mgr;
	const ai::CharacterId id = 1;
	ai::ICharacterPtr entity(new TestEntity(id));
	ai::AIPtr ai(new ai::AI(ai::TreeNodePtr()));
	ai->setCharacter(entity);
	mgr.addAggro(id, 1.0f);
	const ai::EntryPtr& entry = mgr.getHighestEntry();
	ASSERT_TRUE(entry) << "Highest entry not set but aggro was added";
	ASSERT_EQ(id, entry->getCharacterId())<< "Highest entry not what it should be";
	mgr.addAggro(id, 1.0f);
	ASSERT_EQ(1u, mgr.count())<< "Aggrolist contains more entries than expected";
	ASSERT_FLOAT_EQ(2.0f, entry->getAggro())<< "Aggro value not what it should be";
}

TEST_F(AggroTest, testAggroMgr50) {
	doMassTest(50);
}

TEST_F(AggroTest, testAggroMgr500) {
	doMassTest(500);
}

TEST_F(AggroTest, testAggroMgr5000) {
	doMassTest(5000);
}

TEST_F(AggroTest, testAggroMgr10000) {
	doMassTest(10000);
}

TEST_F(AggroTest, testAggroMgrDegradeValue) {
	const float expectedAggro = 1.0f;
	const int seconds = 2;
	const float reduceBySecond = 0.1f;
	ai::AggroMgr mgr;
	const ai::CharacterId id = 1;
	ai::ICharacterPtr entity(new TestEntity(id));
	ai::AIPtr ai(new ai::AI(ai::TreeNodePtr()));
	ai->setCharacter(entity);
	mgr.addAggro(id, expectedAggro);
	const ai::EntryPtr& entry = mgr.getHighestEntry();
	entry->setReduceByValue(reduceBySecond);
	ASSERT_TRUE(entry) << "Highest entry not set but aggro was added";
	ASSERT_EQ(id, entry->getCharacterId())<< "Highest entry not what it should be";
	const float aggro = entry->getAggro();
	ASSERT_FLOAT_EQ(expectedAggro, aggro);
	mgr.update(seconds * 1000);
	const float expected = expectedAggro - seconds * reduceBySecond;
	const float newAggro = entry->getAggro();
	ASSERT_FLOAT_EQ(expected, newAggro);
}
