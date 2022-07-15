/**
 * @file
 */

#pragma once

#include "voxedit-util/modifier/ModifierType.h"

namespace voxedit {

class ModifierPanel {
private:
	bool modifierRadioButton(const char *title, ModifierType type, const char *tooltip = nullptr);

public:
	void update(const char *title);
};

}
