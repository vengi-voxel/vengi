/**
 * @file
 */

#include "LSystem.h"

namespace voxelgenerator {
namespace lsystem {

bool parseRules(const core::String& rulesStr, core::DynamicArray<Rule>& rules) {
	core::Tokenizer tokenizer(rulesStr, " \n");
	while (tokenizer.hasNext()) {
		const core::String block = tokenizer.next();
		if (block != "{") {
			Log::error("Expected '{', but got %s", block.c_str());
			return false;
		}
		if (!tokenizer.hasNext()) {
			Log::error("Expected single char");
			return false;
		}
		const core::String a = tokenizer.next();
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
		rules.push_back(rule);
		if (!tokenizer.hasNext()) {
			Log::error("Expected '}' marker");
			return false;
		}
		const core::String end = tokenizer.next();
		if (end != "}") {
			Log::error("Expected '}', but got %s", end.c_str());
			return false;
		}
	}
	return true;
}

}
}
