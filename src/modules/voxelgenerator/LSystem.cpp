/**
 * @file
 */

#include "LSystem.h"
#include "core/Tokenizer.h"
#include "core/Log.h"

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
