/**
 * @file
 */

#pragma once

#include "ui/Window.h"
#include "ui/ui_widgets.h"
#include "core/Common.h"
#include <unordered_map>
#include "../NoiseData.h"

class NoiseTool;

class NoiseToolWindow: public ui::Window {
private:
	static constexpr int BPP = 4;
	using Super = ui::Window;
	NoiseTool* _noiseTool;
	NoiseData _data;
	tb::TBSelectList* _select = nullptr;
	tb::TBSelectDropdown* _noiseType = nullptr;
	tb::TBSelectItemSourceList<tb::TBGenericStringItem> _noiseTypes;

	int _noiseWidth = 768;
	int _noiseHeight = 1024;
	const int _graphHeight = 65;
	uint8_t *_noiseBuffer = nullptr;
	uint8_t *_graphBuffer = nullptr;
	uint8_t *_graphBufferBackground = nullptr;

	float getNoise(int x, int y);
	int index(int x, int y) const;
	void generateImage();
public:
	NoiseToolWindow(NoiseTool* tool);
	~NoiseToolWindow();
	bool init();

	bool OnEvent(const tb::TBWidgetEvent &ev) override;
	void OnDie() override;
};

inline int NoiseToolWindow::index(int x, int y) const {
	return x * BPP + y * _noiseWidth * BPP;
}
