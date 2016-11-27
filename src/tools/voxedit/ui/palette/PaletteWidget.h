#pragma once

#include "ui/Widget.h"
#include "voxel/polyvox/Voxel.h"

class PaletteWidget: public ui::Widget {
private:
	using Super = ui::Widget;
protected:
	int _width = 0;
	int _height = 0;
	int _padding = 0;
	int _amountX = 0;
	bool _dirty = true;
	int _value = 0;
	voxel::VoxelType _voxelType = voxel::VoxelType::Grass1;
public:
	UIWIDGET_SUBCLASS(PaletteWidget, Super);

	PaletteWidget();
	~PaletteWidget();

	voxel::VoxelType voxelType() const;
	void markAsClean();
	bool isDirty() const;

	void SetValue(int value) override;
	int GetValue() override;
	tb::PreferredSize OnCalculatePreferredContentSize(const tb::SizeConstraints &constraints) override;
	void OnPaint(const PaintProps &paint_props) override;
	void OnInflate(const tb::INFLATE_INFO &info) override;
	bool OnEvent(const tb::TBWidgetEvent &ev) override;
};

inline voxel::VoxelType PaletteWidget::voxelType() const {
	return _voxelType;
}

inline int PaletteWidget::GetValue() {
	return (int) _value;
}

inline void PaletteWidget::SetValue(int value) {
	_value = value;
}

inline void PaletteWidget::markAsClean() {
	_dirty = false;
}

inline bool PaletteWidget::isDirty() const {
	return _dirty;
}
