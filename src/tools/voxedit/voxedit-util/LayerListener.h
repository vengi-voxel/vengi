/**
 * @file
 */

#pragma once

namespace voxedit {

struct Layer;

/**
 * @brief A listener interface for the LayerManager
 * @sa LayerManager
 */
class LayerListener {
public:
	virtual ~LayerListener() {}

	virtual void onLayerHide(int layerId) {}
	virtual void onLayerShow(int layerId) {}
	virtual void onActiveLayerChanged(int old, int active) {}
	virtual void onLayerAdded(int layerId, const Layer& layer, voxel::RawVolume* volume, const voxel::Region& region) {}
	virtual void onLayerDeleted(int layerId, const Layer& layer) {}
};

}
