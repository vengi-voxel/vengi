#pragma once

#include "MapView.h"
#include "RconMapItem.h"

namespace rcon {

class RconAIDebugger;

class RconMapView: public ai::debug::MapView {
private:
	using Super = ai::debug::MapView;
public:
	RconMapView(RconAIDebugger& debugger) :
			Super((ai::debug::AIDebugger&)debugger) {
	}

	ai::debug::MapItem* createMapItem(const ai::AIStateWorld& state) {
		return new RconMapItem(nullptr, state, _debugger);
	}
};

}
