/**
 * @file
 */

#pragma once

#include "core/IComponent.h"

namespace audio {

class SoundManager : public core::IComponent {
public:
	bool init() override;
	void construct() override;
	void shutdown() override;
};

}
