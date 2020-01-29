/**
 * @file
 */

#pragma once

#include "Layer.h"
#include "core/IComponent.h"
#include "LayerListener.h"
#include <set>
#include <unordered_map>

namespace voxedit {

/**
 * @brief Manages the layers in a scene and notifies the registered listeners
 * according to the changes
 */
class LayerManager : public core::IComponent {
private:
	int _activeLayer = 0;
	Layers _layers;
	std::set<LayerListener*> _listeners;

	bool isValidLayerId(int layerId) const;
	bool hasValidLayerAfter(int layerId, int& id) const;
	bool hasValidLayerBefore(int layerId, int& id) const;
public:
	void construct() override;
	bool init() override;
	void shutdown() override;

	/**
	 * @note Does not take over ownership
	 */
	void registerListener(LayerListener* listener);
	void unregisterListener(LayerListener* listener);

	bool duplicate(int layerId);
	bool rename(int layerId, const core::String& name);
	bool moveUp(int layerId);
	bool moveDown(int layerId);

	bool findNewActiveLayer();
	int activeLayer() const;
	bool setActiveLayer(int layerId);
	const Layers& layers() const;
	const Layer& layer(int layerId) const;
	Layer& layer(int layerId);
	int validLayers() const;
	int nextLockedLayer(int last = -1) const;

	/**
	 * @brief Loops over the group of the current active layer
	 */
	void foreachGroupLayer(std::function<void(int)> func);

	bool isLocked(int layerId) const;
	bool isVisible(int layerId) const;
	void hideLayer(int layerId, bool hide);
	void lockLayer(int layerId, bool lock);
	bool deleteLayer(int layerId, bool force = false);
	int addLayer(const char *name, bool visible, voxel::RawVolume* volume, const glm::ivec3& pivot = glm::zero<glm::ivec3>());
	bool activateLayer(int layerId, const char *name, bool visible, voxel::RawVolume* volume, const voxel::Region& region, const glm::ivec3& pivot = glm::zero<glm::ivec3>());
	void addMetadata(int layerId, const LayerMetadata& metadata);
	const LayerMetadata& metadata(int layerId) const;
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
