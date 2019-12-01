/**
 * @file
 */

#pragma once

namespace voxel {
class RawVolume;
class Region;
}

namespace voxedit {

struct Layer;

/**
 * @brief A listener interface for the LayerManager
 * @sa LayerManager
 */
class LayerListener {
public:
	virtual ~LayerListener() {}

	virtual void onLayerChanged(int layerId) {}
	virtual void onLayerDuplicate(int layerId) {}
	virtual void onLayerSwapped(int layerId1, int layerId2) {}
	virtual void onLayerHide(int layerId) {}
	virtual void onLayerShow(int layerId) {}
	virtual void onLayerUnlocked(int layerId) {}
	virtual void onLayerLocked(int layerId) {}
	virtual void onActiveLayerChanged(int old, int active) {}
	virtual void onLayerAdded(int layerId, const Layer& layer, voxel::RawVolume* volume, const voxel::Region& region) {}
	virtual void onLayerDeleted(int layerId, const Layer& layer) {}
};

}
