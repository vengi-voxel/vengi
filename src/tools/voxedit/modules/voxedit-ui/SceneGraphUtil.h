/**
 * @file
 */

#pragma once

#include "IconsLucide.h"
#include "scenegraph/SceneGraphNodeType.h"

namespace voxedit {

inline const char *nodeIcon(scenegraph::SceneGraphNodeType type) {
	switch (type) {
	case scenegraph::SceneGraphNodeType::ModelReference:
		return ICON_LC_CODESANDBOX;
	case scenegraph::SceneGraphNodeType::Model:
		return ICON_LC_BOXES;
	case scenegraph::SceneGraphNodeType::Point:
		return ICON_LC_POINTER;
	case scenegraph::SceneGraphNodeType::Root:
	case scenegraph::SceneGraphNodeType::Group:
		return ICON_LC_GROUP;
	case scenegraph::SceneGraphNodeType::Camera:
		return ICON_LC_CAMERA;
	case scenegraph::SceneGraphNodeType::Unknown:
		return ICON_LC_CIRCLE_QUESTION_MARK;
	case scenegraph::SceneGraphNodeType::AllModels:
	case scenegraph::SceneGraphNodeType::All:
	case scenegraph::SceneGraphNodeType::Max:
		break;
	}
	return "";
}

} // namespace voxedit