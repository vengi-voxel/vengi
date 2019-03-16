/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Widget.h"
#include "voxel/polyvox/Voxel.h"

class PaletteWidget: public ui::turbobadger::Widget {
private:
	using Super = ui::turbobadger::Widget;
protected:
	int _width = 0;
	int _height = 0;
	int _padding = 0;
	int _amountX = 0;
	int _amountY = 0;
	bool _dirty = true;
	// the palette index
	int _value = 0;
	uint8_t _voxelColorIndex = 0u;
public:
	UIWIDGET_SUBCLASS(PaletteWidget, Super);

	PaletteWidget();

	voxel::VoxelType voxelType() const;
	void markAsClean();
	bool isDirty() const;

	void setVoxelColor(uint8_t index);
	void setValue(int value) override;
	int getValue() const override;
	tb::PreferredSize onCalculatePreferredContentSize(const tb::SizeConstraints &constraints) override;
	void onPaint(const PaintProps &paint_props) override;
	void onInflate(const tb::INFLATE_INFO &info) override;
	void onResized(int oldWidth, int oldHeight) override;
	bool onEvent(const tb::TBWidgetEvent &ev) override;
};

UIWIDGET_FACTORY(PaletteWidget, tb::TBValue::TYPE_INT, tb::WIDGET_Z_TOP)

inline void PaletteWidget::setVoxelColor(uint8_t index) {
	_voxelColorIndex = index;
}

inline int PaletteWidget::getValue() const {
	return _value;
}

inline void PaletteWidget::markAsClean() {
	_dirty = false;
}

inline bool PaletteWidget::isDirty() const {
	return _dirty;
}
