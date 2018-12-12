/**
 * @file
 */

#pragma once

#include "ui/turbobadger/TurboBadger.h"
#include "../../NoiseData.h"

class NoiseTool;

class NoiseItem: public tb::TBGenericStringItem {
public:
	NoiseItem(const char* str, const tb::TBID& id, const NoiseData& data) :
			tb::TBGenericStringItem(str, id), _data(data) {
	}
	const NoiseData& data() const {
		return _data;
	}

private:
	NoiseData _data;
};

class NoiseItemSource: public tb::TBSelectItemSourceList<NoiseItem> {
private:
	NoiseTool* _tool;

public:
	NoiseItemSource(NoiseTool* tool);

	virtual bool Filter(int index, const char *filter);
	virtual tb::TBWidget *CreateItemWidget(int index, tb::TBSelectItemViewer *viewer);
};

class NoiseDataItemWidget: public tb::TBLayout {
private:
	using Super = tb::TBLayout;
	NoiseItemSource *_source;
	int _index;
	NoiseTool* _tool;
public:
	NoiseDataItemWidget(NoiseTool* tool, NoiseItem *item, NoiseItemSource *source, int index);

	bool OnEvent(const tb::TBWidgetEvent &ev) override;
};

class NoiseDataList : public tb::TBLayout {
private:
	using Super = tb::TBLayout;
	tb::TBSelectList* _select;
public:
	UIWIDGET_SUBCLASS(NoiseDataList, Super);

	NoiseDataList();
	~NoiseDataList();

	bool OnEvent(const tb::TBWidgetEvent &ev) override;
};

UIWIDGET_FACTORY(NoiseDataList, tb::TBValue::TYPE_NULL, tb::WIDGET_Z_TOP)



