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
	NoiseTool* _noiseTool;

	NoiseData _data;

	long _time = 0l;
	long _autoUpdate = 0l;
	tb::TBWidget* _editorContainer = nullptr;
	tb::TBLayout* _imageLayout = nullptr;
	tb::TBScrollContainer* _imageScrollContainer = nullptr;
	ImageWidget* _graphImage = nullptr;
	tb::TBWidget* _graphBegin = nullptr;
	uint8_t *_autoBuffer = nullptr;
	int _autoWidth = 0;
	int _autoHeight = 0;
	bool _dirtyParameters = true;
	std::string _lastActiveImage;
	std::unordered_map<std::string, tb::TBImage> _graphs;
	tb::TBSelectItemSourceList<tb::TBGenericStringItem> _noiseTypes;

	void fillBuffer(NoiseType noiseType, int width, int height, int components, int cols, int rows, int widgetWidth);
	float getNoise(NoiseType noiseType, int x, int y);
	void allInOne2DNoise();
	void makeSingle2DNoise(bool append, NoiseType noiseType);
	void cleanup(const tb::TBStr& idStr);
	void addImage(const tb::TBStr& idStr, bool append, uint8_t* buffer, int width, int height);
	void addGraph(const std::string& idStr, uint8_t* buffer, int width, int height);
	void removeImage(tb::TBWidget *image);
	void generateImage();
	void activateGraph();
	std::string getCaption(tb::TBWidget* widget) const;
	std::string getGraphName(const char* idStr) const;
public:
	NoiseToolWindow(NoiseTool* tool);
	~NoiseToolWindow();
	bool init();
	void update(long dt);

	bool OnEvent(const tb::TBWidgetEvent &ev) override;
	void OnDie() override;
};
