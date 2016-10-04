#pragma once

#include "AIDebugger.h"
#include "RconMapView.h"

namespace rcon {

class RconAIDebugger: public ai::debug::AIDebugger {
private:
	using Super = ai::debug::AIDebugger;
protected:
	ai::debug::MapView* createMapWidget() override {
		return new RconMapView(*this);
	}

public:
	RconAIDebugger(ai::debug::AINodeStaticResolver& resolver) :
			Super(resolver) {
	}
};

}
