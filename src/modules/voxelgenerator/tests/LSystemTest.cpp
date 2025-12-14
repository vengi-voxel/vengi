/**
 * @file
 */

#include "voxelgenerator/LSystem.h"
#include "app/tests/AbstractTest.h"

namespace voxelgenerator {
namespace lsystem {

class LSystemTests : public app::AbstractTest {};

TEST_F(LSystemTests, testParse) {
	const core::String rulesString = R"(
		{
			F
			F+[!+F-F-FL]-[!-F+F+FL]>[!F<F<FL]<[!<F>F>FL]
		}
		{
			F
			F+[!+F-F-FL]-[!-F+F+FL]>[!F<F<FL]<[!<F>F>FL]
		}
	)";
	core::DynamicArray<Rule> rules;
	ASSERT_TRUE(parseRules(rulesString, rules));
	ASSERT_EQ(2u, rules.size());
}

TEST_F(LSystemTests, testTemplates) {
	const core::DynamicArray<LSystemTemplate> &templates = defaultTemplates();
	ASSERT_FALSE(templates.empty());
	for (const auto& t : templates) {
		LSystemState state;
		prepareState(t.config, state);
		ASSERT_FALSE(state.sentence.empty()) << "Template " << t.name << " failed to generate sentence";
	}
}

}
}
