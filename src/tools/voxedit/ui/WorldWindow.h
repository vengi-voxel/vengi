/**
 * @file
 */

#pragma once

#include "ui/Window.h"
#include "core/Common.h"
#include "voxel/WorldContext.h"

class EditorScene;

namespace voxedit {

class WorldWindow: public ui::Window {
private:
	EditorScene* _scene;
	voxel::WorldContext _ctx;
public:
	WorldWindow(ui::Window* window, EditorScene* scene);

	bool OnEvent(const tb::TBWidgetEvent &ev) override;
};

}
