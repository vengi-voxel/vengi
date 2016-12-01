/**
 * @file
 */

#pragma once

#include "ui/Window.h"
#include "core/Common.h"

class NoiseToolWindow: public ui::Window {
private:
	tb::TBWidget* _editorContainer = nullptr;

	void make2DNoise(bool append, bool gray, bool seamless, bool alpha, float amplitude,
			float frequency, int octaves, float persistence);	void cleanup(const tb::TBStr& idStr);
	void addImage(const tb::TBStr& idStr, bool append, uint8_t* buffer, int width, int height);
	void removeImage(TBWidget *image);
	void generateImage();
public:
	NoiseToolWindow(ui::UIApp* tool);
	bool init();

	bool OnEvent(const tb::TBWidgetEvent &ev) override;
	void OnDie() override;
};
