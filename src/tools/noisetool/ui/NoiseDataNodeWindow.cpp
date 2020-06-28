/**
 * @file
 */

#include "NoiseDataNodeWindow.h"

#include "../NoiseTool.h"
#include "noisedata/NoiseDataNodeWidget.h"
#include "noisedata/NoiseDataItemWidget.h"
#include "ui/turbobadger/UIApp.h"

NoiseDataNodeWindow::NoiseDataNodeWindow(NoiseTool* tool) :
		Super(tool), _noiseTool(tool) {
}

bool NoiseDataNodeWindow::init() {
	if (!loadResourceFile("ui/window/noisetool-nodes.tb.txt")) {
		Log::error("Failed to init the main window: Could not load the ui definition");
		return false;
	}

	_nodesWidget = getWidgetByType<tb::TBWidget>("nodes");
	if (_nodesWidget == nullptr) {
		Log::error("Could not find nodes widget");
		return false;
	}

	// TEST TEST TEST
	NoiseItemSource* source = _noiseTool->noiseItemSource();
	const int n = source->getNumItems();
	for (int index = 0; index < n; ++index) {
		NoiseItem* item = source->getItem(index);
		NoiseDataNodeWidget *itemWidget = new NoiseDataNodeWidget(item);
		_nodesWidget->getContentRoot()->addChild(itemWidget);
	}
	return true;
}

bool NoiseDataNodeWindow::onEvent(const tb::TBWidgetEvent &ev) {
	return Super::onEvent(ev);
}
