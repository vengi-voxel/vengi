/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Window.h"

class NoiseTool;
class NoiseDataItemWidget;

class NoiseDataNodeWindow: public ui::Window {
private:
	using Super = ui::Window;

	tb::TBWidget* _nodesWidget = nullptr;
	NoiseTool* _noiseTool;
public:
	NoiseDataNodeWindow(NoiseTool* tool);
	bool init();

	bool OnEvent(const tb::TBWidgetEvent &ev) override;
};
