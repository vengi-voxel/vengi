#include "XMLTreeLoaderTest.h"
#include <SimpleAI.h>
#include <tree/loaders/xml/XMLTreeLoader.h>

namespace {
const char *TREE = "<?xml version=\"1.0\" standalone=\"no\" ?>"
	"<trees>"
		"<tree name=\"example1\">"
			"<node type=\"PrioritySelector\" name=\"root\">"
				"<node type=\"Idle{3000}\" name=\"idle\" condition=\"HasEnemies{3}\" />"
			"</node>"
		"</tree>"
		"<tree name=\"example2\">"
			"<node type=\"PrioritySelector\" name=\"root\">"
				"<node type=\"Idle{3000}\" name=\"idle\" />"
				"<node type=\"Wander\" name=\"wander\" />"
			"</node>"
		"</tree>"
	"</trees>";
}

class XMLTreeLoaderTest: public TestSuite {
};

TEST_F(XMLTreeLoaderTest, testLoad) {
	ai::AIRegistry registry;
	ai::XMLTreeLoader loader(registry);
	ASSERT_TRUE(loader.init(TREE)) << loader.getError();
	const ai::TreeNodePtr& tree = loader.load("example1");
	ASSERT_NE(nullptr, tree.get()) << "Could not find the espected behaviour";
	ASSERT_EQ("root", tree->getName()) << "unexpected root node name";
	const ai::TreeNodes& children = tree->getChildren();
	const int childrenAmount = children.size();
	ASSERT_EQ(1, childrenAmount) << "expected amount of children";
	ASSERT_EQ("idle", children[0]->getName()) << "unexpected child node name";
	ASSERT_NE(nullptr, children[0]->getCondition()) << "condition not parsed";
	ASSERT_EQ("HasEnemies", children[0]->getCondition()->getName()) << "unexpected condition name";
	const ai::TreeNodePtr& example2 = loader.load("example2");
	ASSERT_NE(nullptr, example2.get()) << "Could not find the espected behaviour";
	ASSERT_EQ("root", example2->getName()) << "unexpected root node name";
	const ai::TreeNodes& children2 = example2->getChildren();
	const int childrenAmount2 = children2.size();
	ASSERT_EQ(2, childrenAmount2) << "expected amount of children";
}
