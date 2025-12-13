/**
 * @file
 */

#include "LSystem.h"
#include "core/Tokenizer.h"
#include "core/Log.h"

namespace voxelgenerator {
namespace lsystem {

void prepareState(const LSystemConfig &conf, LSystemState &state) {
	state.sentence = conf.axiom;
	state.position = conf.position;
	state.angle = conf.angle;
	state.length = conf.length;
	state.width = conf.width;
	state.widthIncrement = conf.widthIncrement;
	state.leafRadius = conf.leafRadius;

	for (int i = 0; i < conf.iterations; i++) {
		core::String nextSentence = "";

		for (size_t j = 0; j < state.sentence.size(); ++j) {
			const char current = state.sentence[j];
			bool found = false;
			for (const auto &rule : conf.rules) {
				if (rule.a == current) {
					found = true;
					nextSentence += rule.b;
					break;
				}
			}
			if (!found) {
				nextSentence += current;
			}
		}

		state.sentence = nextSentence;
	}
}


bool parseRules(const core::String& rulesStr, core::DynamicArray<Rule>& rules) {
	core::Tokenizer tokenizer(rulesStr, " \n");
	while (tokenizer.hasNext()) {
		const core::String block = tokenizer.next();
		if (block != "{") {
			Log::error("Expected '{', but got %s", block.c_str());
			return false;
		}
		while (tokenizer.hasNext()) {
			const core::String a = tokenizer.next();
			if (a == "}") {
				break;
			}
			if (a.size() != 1) {
				Log::error("Expected single char, but got '%s'", a.c_str());
				return false;
			}
			if (!tokenizer.hasNext()) {
				Log::error("Expected lsystem rule string");
				return false;
			}
			Rule rule;
			rule.a = a[0];
			rule.b = tokenizer.next();
			if (rule.b == "}") {
				Log::error("Expected lsystem rule string, but got '}'");
				return false;
			}
			rules.push_back(rule);
		}
	}
	return true;
}

}
}
