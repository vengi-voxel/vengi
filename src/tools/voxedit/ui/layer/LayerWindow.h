/**
 * @file
 */

#pragma once

#include "AbstractLayerPopupWindow.h"
#include "voxel/Region.h"

namespace voxedit {

enum class LayerWindowType {
	NewScene,
	Create,
	Edit
};

constexpr const int MaxVolumeSize = 128;
struct LayerSettings {
	core::String name;
	glm::ivec3 position;
	glm::ivec3 size;

	inline void reset() {
		position = glm::ivec3(0);
		size = glm::ivec3(32);
	}

	inline voxel::Region region() {
		const voxel::Region region(position, position + size - 1);
		if (!region.isValid()) {
			reset();
			return voxel::Region {position, position + size - 1};
		}
		const glm::ivec3& dim = region.getDimensionsInCells();
		if (dim.x >= MaxVolumeSize || dim.y >= MaxVolumeSize || dim.z >= MaxVolumeSize) {
			reset();
			return voxel::Region {position, position + size - 1};
		}
		return region;
	}
};

class LayerWindowSettings {
public:
	LayerWindowSettings() {
	}
	LayerWindowSettings(LayerWindowType type, const tb::TBID &iconSkin) :
			type(type), iconSkin(iconSkin) {
	}

	/** @brief The type of response for the message. */
	LayerWindowType type = LayerWindowType::Create;
	tb::TBID iconSkin;
};

class LayerWindow : public AbstractLayerPopupWindow {
private:
	using Super = AbstractLayerPopupWindow;
	LayerSettings& _layerSettings;
	LayerWindowSettings _layerWindowSettings;

	void checkSize();
protected:
	void onShow() override;
public:
	LayerWindow(tb::TBWidget *target, const tb::TBID &id, LayerSettings& layerSettings, LayerWindowSettings* settings = nullptr);
	virtual ~LayerWindow() {}

	bool onEvent(const tb::TBWidgetEvent &ev) override;
};

}
