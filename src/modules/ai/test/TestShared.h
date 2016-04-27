#pragma once
#include <gtest/gtest.h>
#include <SimpleAI.h>
#include "TestEntity.h"
#include <sstream>

class TestSuite: public ::testing::Test {
protected:
	ai::AIRegistry _registry;
	ai::GroupMgr _groupManager;

	std::string printAggroList(ai::AggroMgr& aggroMgr) const {
		const ai::AggroMgr::Entries& e = aggroMgr.getEntries();
		if (e.empty())
			return "empty";

		std::stringstream s;
		for (ai::AggroMgr::Entries::const_iterator i = e.begin(); i != e.end(); ++i) {
			const ai::Entry& ep = *i;
			s << ep.getCharacterId() << "=" << ep.getAggro() << ", ";
		}
		const ai::EntryPtr& highest = aggroMgr.getHighestEntry();
		s << "highest: " << highest->getCharacterId() << "=" << highest->getAggro();
		return s.str();
	}

	virtual void SetUp() override;
	virtual void TearDown() override;
};
