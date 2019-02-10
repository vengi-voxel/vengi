/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Window.h"

class NoiseTool;
class NoiseDataItemWidget;

class NoiseDataNodeWindow: public ui::turbobadger::Window {
private:
	using Super = ui::turbobadger::Window;

	tb::TBWidget* _nodesWidget = nullptr;
	NoiseTool* _noiseTool;
public:
	NoiseDataNodeWindow(NoiseTool* tool);
	bool init();

	bool onEvent(const tb::TBWidgetEvent &ev) override;
};
