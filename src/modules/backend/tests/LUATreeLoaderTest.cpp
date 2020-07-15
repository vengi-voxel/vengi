#include "TestShared.h"
#include "backend/entity/ai/tree/loaders/lua/LUATreeLoader.h"

namespace {
const char *TREE = "function init ()"
		"local example = AI.createTree(\"example\")"
		"local rootNodeExample1 = example:createRoot(\"PrioritySelector\", \"root1\")"
		"rootNodeExample1:addNode(\"Idle{3000}\", \"idle3000_1\"):setCondition(\"True\")"
		"local rootNodeExample2 = AI.createTree(\"example2\"):createRoot(\"PrioritySelector\", \"root2\")"
		"rootNodeExample2:addNode(\"Idle{3000}\", \"idle3000_2\"):setCondition(\"True\")"
		"rootNodeExample2:addNode(\"Steer{0.6,0.4}(GroupFlee{2},Wander{1})\", \"wander\")"
		"end";
}

namespace backend {

class LUATreeLoaderTest: public TestSuite {
protected:
	AIRegistry _registry;
	LUATreeLoader _loader;

public:
	LUATreeLoaderTest() :
			_loader(_registry) {
	}

	void SetUp() override {
		TestSuite::SetUp();
		ASSERT_TRUE(_loader.init(TREE)) << _loader.getError();
	}

	void TearDown() override {
		TestSuite::TearDown();
		_loader.shutdown();
	}
};

TEST_F(LUATreeLoaderTest, testLoadExample) {
	const TreeNodePtr& tree = _loader.load("example");
	ASSERT_NE(nullptr, tree.get()) << "Could not find the expected behaviour";
	ASSERT_EQ("root1", tree->getName()) << "unexpected root node name";
	const TreeNodes& children = tree->getChildren();
	const int childrenAmount = children.size();
	ASSERT_EQ(1, childrenAmount) << "expected amount of children";
	ASSERT_EQ("idle3000_1", children[0]->getName()) << "unexpected child node name";
	ASSERT_EQ("True", children[0]->getCondition()->getName()) << "unexpected condition name";
}

TEST_F(LUATreeLoaderTest, testLoadExample2) {
	const TreeNodePtr& tree = _loader.load("example2");
	ASSERT_NE(nullptr, tree.get()) << "Could not find the expected behaviour";
	ASSERT_EQ("root2", tree->getName()) << "unexpected root node name";
	const TreeNodes& children = tree->getChildren();
	const int childrenAmount = children.size();
	ASSERT_EQ(2, childrenAmount) << "expected amount of children";
	ASSERT_EQ("idle3000_2", children[0]->getName()) << "unexpected child node name";
	ASSERT_EQ("True", children[0]->getCondition()->getName()) << "unexpected condition name";
	ASSERT_EQ("wander", children[1]->getName()) << "unexpected child node name";
	ASSERT_EQ("True", children[0]->getCondition()->getName()) << "unexpected condition name";
}

}
