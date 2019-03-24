/**
 * @file
 */

#pragma once

namespace voxedit {

struct Layer;

class SceneListener {
public:
	virtual ~SceneListener() {}

	virtual void onLayerHide(int layerId) {}
	virtual void onLayerShow(int layerId) {}
	virtual void onActiveLayerChanged(int old, int active) {}
	virtual void onLayerAdded(int layerId, const Layer& layer) {}
	virtual void onLayerDeleted(int layerId) {}
};

}
