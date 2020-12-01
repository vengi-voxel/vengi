/**
 * @file
 */

#pragma once

#include "AbstractLayerPopupWindow.h"
#include "voxedit-util/layer/LayerSettings.h"
#include "voxel/Region.h"

namespace voxedit {

enum class LayerWindowType {
	NewScene,
	Create,
	Edit
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
