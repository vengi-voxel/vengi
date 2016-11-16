#pragma once

#include "ui/TurboBadger.h"

namespace voxedit {

class RuleItem: public tb::TBGenericStringItem {
private:
	char _character;
public:
	RuleItem(const char *str, char character) :
			tb::TBGenericStringItem(str, TBIDC("rule_item")), _character(character) {
	}

	inline char character() const {
		return _character;
	}
};

}
