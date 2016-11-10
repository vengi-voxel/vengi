/**
 * @file
 */

#pragma once

#include "ui/Window.h"
#include "core/Common.h"

class NoiseParametersWindow: public ui::Window {
private:
	void make2DNoise(bool append, bool gray, bool seamless, bool alpha, float amplitude,
			float frequency, int octaves, float persistence);	void cleanup(const tb::TBStr& idStr);
	void addImage(const tb::TBStr& idStr, bool append, uint8_t* buffer, int width, int height);
public:
	NoiseParametersWindow(ui::UIApp* tool);
	bool init();

	bool OnEvent(const tb::TBWidgetEvent &ev) override;
	void OnDie() override;
};
