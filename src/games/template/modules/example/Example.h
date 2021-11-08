/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/SharedPtr.h"

namespace tmpl {

class Example : public core::IComponent {
public:
	bool init() override;
	void shutdown() override;

	void update();
};

typedef core::SharedPtr<Example> ExamplePtr;

}
