/**
 * @file
 */

#pragma once

#include "core/IComponent.h"

namespace tmpl {

class Example : public core::IComponent {
public:
	bool init() override;
	void shutdown() override;
};

}
