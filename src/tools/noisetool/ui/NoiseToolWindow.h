/**
 * @file
 */

#pragma once

#include "ui/Window.h"
#include "ui/ui_widgets.h"
#include "core/Common.h"

enum class NoiseType {
	simplexNoise,
	ridgedNoise,
	flowNoise,
	fbm,
	fbmCascade,
	fbmAnalyticalDerivatives,
	flowNoiseFbm,
	ridgedMFTime,
	ridgedMF,
	ridgedMFCascade,
	ridgedMFScaled,
	iqNoise,
	iqNoiseScaled,
	analyticalDerivatives,
	noiseCurlNoise,

	Max
};

class NoiseToolWindow: public ui::Window {
private:
	float _frequency = 0.0f;
	float _offset = 0.0f;
	float _lacunarity = 0.0f;
	int _octaves = 0;
	float _gain = 0.0f;

	NoiseType _noiseType = NoiseType::Max;

	long _time = 0l;
	long _autoUpdate = 0l;
	tb::TBWidget* _editorContainer = nullptr;
	tb::TBLayout* _imageLayout = nullptr;
	tb::TBScrollContainer* _imageCcrollContainer = nullptr;
	ImageWidget* _graphImage = nullptr;
	tb::TBWidget* _graphBegin = nullptr;
	uint8_t *_autoBuffer = nullptr;
	int _autoWidth = 0;
	int _autoHeight = 0;
	bool _dirtyParameters = true;

	tb::TBSelectItemSourceList<tb::TBGenericStringItem> _noiseTypes;

	void fillBuffer(NoiseType noiseType, int width, int height, int components, int cols, int rows, int widgetWidth);
	float getNoise(NoiseType noiseType, int x, int y);
	void allInOne2DNoise();
	void makeSingle2DNoise(bool append, NoiseType noiseType);
	void cleanup(const tb::TBStr& idStr);
	void addImage(const tb::TBStr& idStr, bool append, uint8_t* buffer, int width, int height);
	void addGraph(const tb::TBStr& idStr, uint8_t* buffer, int width, int height);
	void removeImage(tb::TBWidget *image);
	void generateImage();
public:
	NoiseToolWindow(ui::UIApp* tool);
	~NoiseToolWindow();
	bool init();
	void update(long dt);

	bool OnEvent(const tb::TBWidgetEvent &ev) override;
	void OnDie() override;
};
