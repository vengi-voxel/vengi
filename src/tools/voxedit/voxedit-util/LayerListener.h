/**
 * @file
 */

#pragma once

namespace voxedit {

struct Layer;

class LayerListener {
public:
	virtual ~LayerListener() {}

	virtual void onLayerHide(int layerId) {}
	virtual void onLayerShow(int layerId) {}
	virtual void onActiveLayerChanged(int old, int active) {}
	virtual void onLayerAdded(int layerId, const Layer& layer, voxel::RawVolume* volume) {}
	virtual void onLayerDeleted(int layerId) {}
};

}
