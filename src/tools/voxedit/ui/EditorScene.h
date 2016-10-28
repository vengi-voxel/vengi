#pragma once

#include "ui/TurboBadger.h"
#include "frontend/RawVolumeRenderer.h"
#include "video/Camera.h"
#include "frontend/Axis.h"

class EditorScene: public ui::Widget {
private:
	using Super = ui::Widget;
	video::Camera _camera;
	frontend::Axis _axis;
	core::VarPtr _rotationSpeed;
	frontend::RawVolumeRenderer _rawVolumeRenderer;

	float _cameraSpeed = 0.1f;

	voxel::Voxel _currentVoxel;

	bool _dirty = false;
	bool _extract = false;
	bool _renderAxis = true;
	uint8_t _moveMask = 0;

	int _mouseX = 0;
	int _mouseY = 0;

	enum class Action {
		None,
		PlaceVoxel,
		CopyVoxel,
		DeleteVoxel
	};
	Action _action = Action::None;

	bool isDirty() const;
	void executeAction(int32_t x, int32_t y);
public:
	UIWIDGET_SUBCLASS(EditorScene, tb::TBWidget);

	EditorScene();
	~EditorScene();

	bool saveModel(std::string_view file);

	bool loadModel(std::string_view file);

	bool newModel(bool force = false);

	virtual bool OnEvent(const tb::TBWidgetEvent &ev) override;

	virtual void OnPaint(const PaintProps &paint_props) override;

	virtual void OnResized(int oldWidth, int oldHeight) override;

	virtual void OnInflate(const tb::INFLATE_INFO &info) override;
};

inline bool EditorScene::isDirty() const {
	return _dirty;
}
