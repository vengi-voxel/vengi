/**
 * @file
 */

#pragma once

namespace voxedit {

enum GizmoOperation {
	GizmoOperation_Translate = 1,
	GizmoOperation_Rotate = 2,
	GizmoOperation_Scale = 4,
	GizmoOperation_Bounds = 8
};

}
