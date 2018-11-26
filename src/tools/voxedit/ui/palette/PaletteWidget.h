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
public:
	UIWIDGET_SUBCLASS(PaletteWidget, Super);

	PaletteWidget();
	~PaletteWidget();

	voxel::VoxelType voxelType() const;
	void markAsClean();
	bool isDirty() const;

	void SetValue(int value) override;
	int GetValue() const override;
	tb::PreferredSize OnCalculatePreferredContentSize(const tb::SizeConstraints &constraints) override;
	void OnPaint(const PaintProps &paint_props) override;
	void OnInflate(const tb::INFLATE_INFO &info) override;
	void OnResized(int oldWidth, int oldHeight) override;
	bool OnEvent(const tb::TBWidgetEvent &ev) override;
};

UIWIDGET_FACTORY(PaletteWidget, tb::TBValue::TYPE_INT, tb::WIDGET_Z_TOP)

inline int PaletteWidget::GetValue() const {
	return _value;
}

inline void PaletteWidget::markAsClean() {
	_dirty = false;
}

inline bool PaletteWidget::isDirty() const {
	return _dirty;
}
