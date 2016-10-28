#pragma once

#include "ui/TurboBadger.h"
#include "frontend/RawVolumeRenderer.h"
#include "video/Camera.h"
#include "frontend/Axis.h"

class EditorScene: public ui::Widget {
private:
	using Super = ui::Widget;
	bool _renderAxis = true;
	video::Camera _camera;
	frontend::Axis _axis;
	core::VarPtr _rotationSpeed;
	uint8_t _moveMask = 0;
	float _cameraSpeed = 0.1f;
	frontend::RawVolumeRenderer _rawVolumeRenderer;
	bool _dirty = false;
	bool _extract = false;
	voxel::Voxel _currentVoxel;

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
