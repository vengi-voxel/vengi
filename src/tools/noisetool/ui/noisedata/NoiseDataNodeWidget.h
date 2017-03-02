#pragma once

#include "TurboBadger.h"

class NoiseItem;

class NoiseDataNodeWidget: public tb::TBLayout {
private:
	using Super = tb::TBLayout;
public:
	NoiseDataNodeWidget(NoiseItem *item);

	bool OnEvent(const tb::TBWidgetEvent &ev) override;
};


