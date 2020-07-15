/**
 * @file
 */

#include "TestShared.h"
#include "core/String.h"
#include "core/io/Filesystem.h"
#include <fstream>
#include <streambuf>

namespace backend {

class LUAAIRegistryTest: public TestSuite {
protected:
	LUAAIRegistry _registry;
	static core::String _luaCode;
	const ai::CharacterId _id = 1;
	ICharacterPtr _chr = std::make_shared<TestEntity>(_id);
	const ConditionFactoryContext ctxCondition = ConditionFactoryContext("");
	const FilterFactoryContext ctxFilter = FilterFactoryContext("");
	const SteeringFactoryContext ctxSteering = SteeringFactoryContext("");

	void SetUp() override {
		TestSuite::SetUp();
		if (_luaCode.empty()) {
			_luaCode = _testApp->filesystem()->load("testluaregistry.lua");
			ASSERT_FALSE(_luaCode.empty());
		}
		ASSERT_TRUE(_registry.init()) << "Failed to initialize the lua ai registry";
		ASSERT_TRUE(_registry.evaluate(_luaCode)) << "Failed to load lua script:\n" << _luaCode;
	}

	void TearDown() override {
		TestSuite::TearDown();
		_registry.shutdown();
	}

	void testSteering(const char* steeringName, int n = 1) {
		SCOPED_TRACE(steeringName);
		const SteeringPtr& steering = _registry.createSteering(steeringName, ctxSteering);
		ASSERT_TRUE((bool)steering) << "Could not create lua provided steering";
		const AIPtr& ai = std::make_shared<AI>(TreeNodePtr());
		ai->setCharacter(_chr);
		for (int i = 0; i < n; ++i) {
			steering->execute(ai, 1.0f);
		}
		ASSERT_EQ(1, steering.use_count()) << "Someone is still referencing the LUASteering";
		lua_gc(_registry.getLuaState(), LUA_GCCOLLECT, 0);
		ASSERT_EQ(1, ai.use_count()) << "Someone is still referencing the AI instance";
	}

	void testFilter(const char* filterName, int n = 1) {
		SCOPED_TRACE(filterName);
		const FilterPtr& filter = _registry.createFilter(filterName, ctxFilter);
		ASSERT_TRUE((bool)filter) << "Could not create lua provided filter";
		const AIPtr& ai = std::make_shared<AI>(TreeNodePtr());
		ai->setCharacter(_chr);
		for (int i = 0; i < n; ++i) {
			filter->filter(ai);
		}
		ASSERT_EQ(1, filter.use_count()) << "Someone is still referencing the LUAFilter";
		lua_gc(_registry.getLuaState(), LUA_GCCOLLECT, 0);
		ASSERT_EQ(1, ai.use_count()) << "Someone is still referencing the AI instance";
	}

	void testCondition(const char* conditionName, bool expectedReturnValue, int n = 1) {
		SCOPED_TRACE(conditionName);
		const ConditionPtr& condition = _registry.createCondition(conditionName, ctxCondition);
		ASSERT_TRUE((bool)condition) << "Could not create lua provided condition";
		const AIPtr& ai = std::make_shared<AI>(TreeNodePtr());
		ai->setCharacter(_chr);
		for (int i = 0; i < n; ++i) {
			ASSERT_EQ(expectedReturnValue, condition->evaluate(ai));
		}
		ASSERT_EQ(1, condition.use_count()) << "Someone is still referencing the LUACondition";
		lua_gc(_registry.getLuaState(), LUA_GCCOLLECT, 0);
		ASSERT_EQ(1, ai.use_count()) << "Someone is still referencing the AI instance";
	}

	void testNode(const char* nodeName, ai::TreeNodeStatus status, int n = 1) {
		const TreeNodeFactoryContext ctx = TreeNodeFactoryContext("TreeNodeName", "", True::get());
		return testNode(nodeName, status, ctx, n);
	}

	void testNode(const char* nodeName, ai::TreeNodeStatus status, const TreeNodeFactoryContext &ctx, int n = 1) {
		SCOPED_TRACE(nodeName);
		Zone zone("TestNode");
		const TreeNodePtr& node = _registry.createNode(nodeName, ctx);
		ASSERT_TRUE((bool)node) << "Could not create lua provided node '" << nodeName << "'";
		const AIPtr& ai = std::make_shared<AI>(node);
		EXPECT_EQ(1, ai.use_count()) << "We are holding more references than expected. Here should be the old reference at the moment. Nodename: " << nodeName;
		ai->setCharacter(_chr);
		EXPECT_EQ(1, ai.use_count()) << "We are holding more references than expected. Here should be the old reference at the moment. Nodename: " << nodeName;
		ASSERT_TRUE(zone.addAI(ai));
		EXPECT_EQ(2, ai.use_count()) << "We are holding more references than expected. One is here, one should be in the pending zone add queue. Nodename: " << nodeName;
		ai->setPause(true);
		zone.update(1l);
		EXPECT_EQ(2, ai.use_count()) << "We are holding more references than expected. One is here, one should be in the zone ai collection. Nodename: " << nodeName;
		ai->setPause(false);
		for (int i = 0; i < n; ++i) {
			const ai::TreeNodeStatus executionStatus = node->execute(ai, 1L);
			ASSERT_EQ(status, executionStatus) << "Lua script returned an unexpected TreeNodeStatus value for node: " << nodeName;
		}
		ASSERT_TRUE(zone.removeAI(_id)) << "Nodename: " << nodeName;
		ai->setPause(true);
		zone.update(1l);
		ai->setPause(false);
		ai->setBehaviour(TreeNodePtr());
		EXPECT_EQ(1, node.use_count()) << "Someone is still referencing the LUATreeNode. Nodename: " << nodeName;
		lua_gc(_registry.getLuaState(), LUA_GCCOLLECT, 0);
		EXPECT_EQ(1, ai.use_count()) << "Someone is still referencing the AI instance. Nodename: " << nodeName;
	}
};

core::String LUAAIRegistryTest::_luaCode;

TEST_F(LUAAIRegistryTest, testLuaNode1) {
	testNode("LuaTest", ai::TreeNodeStatus::FINISHED);
}

TEST_F(LUAAIRegistryTest, testLuaNode2) {
	testNode("LuaTest2", ai::TreeNodeStatus::RUNNING);
}

TEST_F(LUAAIRegistryTest, testLuaNode2_100) {
	testNode("LuaTest2", ai::TreeNodeStatus::RUNNING, 100);
}

TEST_F(LUAAIRegistryTest, testCreateInvalidNode) {
	const TreeNodeFactoryContext ctx = TreeNodeFactoryContext("TreeNodeName", "", True::get());
	const TreeNodePtr& node = _registry.createNode("ThisNameDoesNotExist", ctx);
	ASSERT_FALSE((bool)node) << "Created a node for a type that isn't defined";
}

TEST_F(LUAAIRegistryTest, testConditionEvaluationTrue) {
	testCondition("LuaTestTrue", true);
}

TEST_F(LUAAIRegistryTest, testConditionEvaluationTrue_100) {
	testCondition("LuaTestTrue", true, 100);
}

TEST_F(LUAAIRegistryTest, testConditionEvaluationFalse) {
	testCondition("LuaTestFalse", false);
}

TEST_F(LUAAIRegistryTest, testFilterEmpty) {
	testFilter("LuaFilterTest");
}

TEST_F(LUAAIRegistryTest, testFilter_100) {
	testFilter("LuaFilterTest", 100);
}

TEST_F(LUAAIRegistryTest, testSteeringEmpty) {
	testSteering("LuaSteeringTest");
}

}
