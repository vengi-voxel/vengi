/**
 * @file
 */

#pragma once

#include "ui/turbobadger/TurboBadger.h"

class NoiseItem;

class NoiseDataNodeWidget: public tb::TBLayout {
private:
	using Super = tb::TBLayout;

	bool _clicked = false;
	void createNewNodeAtPosition(int x, int y);
	void setItem(NoiseItem* item);
public:
	NoiseDataNodeWidget(NoiseItem *item);

	bool onEvent(const tb::TBWidgetEvent &ev) override;
	void onPaintChildren(const PaintProps &paintProps) override;
};


