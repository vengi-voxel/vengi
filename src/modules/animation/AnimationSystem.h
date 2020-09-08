/**
 * @file
 */

#pragma once

#include "core/IComponent.h"

namespace animation {

class AnimationSystem : public core::IComponent {
public:
	bool init() override;
	void shutdown() override;
};

}
