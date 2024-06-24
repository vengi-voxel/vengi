/**
 * @file
 */

#pragma once

#include "ui/IMGUIApp.h"
#include "video/Camera.h"
#include "render/Axis.h"
#include "Plane.h"
#include "util/Movement.h"

class TestApp: public ui::IMGUIApp {
private:
	using Super = ui::IMGUIApp;
	video::Camera _testAppCamera;
protected:
	bool _cameraMotion = false;
	bool _renderPlane = false;
	bool _renderAxis = true;
	render::Axis _axis;
	render::Plane _plane;
	glm::vec4 _planeColor = core::Color::White();
	util::Movement _movement;
	core::VarPtr _rotationSpeed;
	float _cameraSpeed = 50.0f;

	virtual void doRender() = 0;

	inline void setUICamera() {
		_testAppCamera = video::uiCamera(windowDimension());
	}

	inline void setCameraSpeed(float cameraSpeed) {
		_cameraSpeed = cameraSpeed;
	}

	inline void setCameraMotion(bool cameraMotion) {
		_cameraMotion = cameraMotion;
	}

	inline void setRenderPlane(bool renderPlane, const glm::vec4& color = core::Color::White()) {
		_renderPlane = renderPlane;
		_planeColor = color;
	}

	inline void setRenderAxis(bool renderAxis) {
		_renderAxis = renderAxis;
	}

public:
	TestApp(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider);
	virtual ~TestApp();

	video::Camera& camera();

	virtual app::AppState onConstruct() override;
	virtual app::AppState onInit() override;
	virtual app::AppState onRunning() override;
	virtual void beforeUI() override;
	virtual void onRenderUI() override;
	virtual app::AppState onCleanup() override;
	virtual bool onMouseWheel(float x, float y) override;
	virtual void onWindowResize(void *windowHandle, int windowWidth, int windowHeight) override;
};

inline video::Camera& TestApp::camera() {
	return _testAppCamera;
}
