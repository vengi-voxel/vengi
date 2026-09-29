/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "testcore/TestApp.h"
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

/**
 * @brief Renders a Dear ImGui style preview plus optional demo windows
 */
class TestIMGUI : public TestApp {
private:
	using Super = TestApp;
	bool _showTestWindow = false;
	bool _showMetricsWindow = false;
	bool _showImPlotWindow = false;

	bool _checkbox = true;
	bool _checkboxDisabled = false;
	int _radio = 1;
	int _combo = 1;
	int _listbox = 0;
	float _slider = 0.45f;
	int _sliderInt = 4;
	core::String _input = "Voxel layer";
	glm::vec3 _position{1.0f, 2.0f, 3.0f};
	glm::vec4 _color{0.26f, 0.59f, 0.98f, 1.0f};
	float _progress = 0.62f;

	void doRender() override;
	void renderStylePreview();

public:
	TestIMGUI(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider);

	app::AppState onConstruct() override;
	app::AppState onInit() override;
	void onRenderUI() override;
};
