/**
 * @file
 */

#pragma once

#include "math/Axis.h"
#include "voxedit-util/modifier/ModifierType.h"

namespace voxedit {

class ModifierPanel {
private:
	bool modifierRadioButton(const char *title, ModifierType type, const char *tooltip = nullptr);
	bool mirrorAxisRadioButton(const char *title, math::Axis type);
	void addShapes();
	void addMirrorPlanes();
public:
	void update(const char *title);
};

}
