/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Window.h"
#include "voxel/polyvox/Region.h"

namespace voxedit {

enum class LayerWindowType {
	NewScene,
	Create,
	Edit
};

struct LayerSettings {
	std::string name;
	glm::ivec3 position;
	glm::ivec3 size;

	inline void reset() {
		position = glm::ivec3(0);
		size = glm::ivec3(127);
	}

	inline voxel::Region region() {
		const voxel::Region region(position, position + size);
		if (!region.isValid()) {
			reset();
			return voxel::Region {position, position + size};
		}
		const glm::ivec3& dim = region.getDimensionsInCells();
		if (dim.x * dim.y * dim.z > 512 * 512 * 512) {
			reset();
			return voxel::Region {position, position + size};
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

class LayerWindow : public tb::TBWindow, private tb::TBWidgetListener {
private:
	using Super = tb::TBWindow;
public:
	TBOBJECT_SUBCLASS(LayerWindow, tb::TBWindow);

	LayerWindow(TBWidget *target, const tb::TBID &id, LayerSettings& layerSettings);
	virtual ~LayerWindow();

	bool show(LayerWindowSettings* settings = nullptr);

	TBWidget *getEventDestination() override {
		return _target.get();
	}

	bool onEvent(const tb::TBWidgetEvent &ev) override;
	void onDie() override;

private:
	void addButton(const tb::TBID &id, bool focused);

	void onWidgetDelete(TBWidget *widget) override;
	bool onWidgetDying(TBWidget *widget) override;

	tb::TBWidgetSafePointer _dimmer;
	tb::TBWidgetSafePointer _target;

	LayerSettings& _layerSettings;
};

}
