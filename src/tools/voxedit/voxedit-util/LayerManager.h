/**
 * @file
 */

#pragma once

#include "Layer.h"
#include "core/IComponent.h"
#include "LayerListener.h"
#include <set>

namespace voxedit {

class LayerManager : public core::IComponent {
private:
	int _activeLayer = 0;
	Layers _layers;
	std::set<LayerListener*> _listeners;

	bool isValidLayerId(int layerId) const;
public:
	void construct() override;
	bool init() override;
	void shutdown() override;

	/**
	 * @note Does not take over ownership
	 */
	void registerListener(LayerListener* listener);
	void unregisterListener(LayerListener* listener);

	bool findNewActiveLayer();
	int activeLayer() const;
	bool setActiveLayer(int layerId);
	const Layers& layers() const;
	const Layer& layer(int layerId) const;
	Layer& layer(int layerId);
	int validLayers() const;
	void hideLayer(int layerId, bool hide);
	bool deleteLayer(int layerId, bool force = false);
	int addLayer(const char *name, bool visible, voxel::RawVolume* volume, const glm::ivec3& pivot = glm::zero<glm::ivec3>());
	bool activateLayer(int layerId, const char *name, bool visible, voxel::RawVolume* volume, const voxel::Region& region, const glm::ivec3& pivot = glm::zero<glm::ivec3>());
	int maxLayers() const;
};

inline int LayerManager::activeLayer() const {
	return _activeLayer;
}

inline const Layers& LayerManager::layers() const {
	return _layers;
}

inline int LayerManager::maxLayers() const {
	return (int)_layers.size();
}

}
