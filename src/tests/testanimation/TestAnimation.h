/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "animation/Character.h"
#include "animation/CharacterCache.h"
#include "animation/CharacterRenderer.h"
#include "attrib/ShadowAttributes.h"
#include "stock/StockDataProvider.h"
#include "stock/Inventory.h"
#include <vector>
#include <string>

/**
 * @brief Renders a character animation
 */
class TestAnimation: public TestApp {
private:
	using Super = TestApp;

	animation::Character _character;
	animation::CharacterCachePtr _characterCache;
	animation::CharacterRenderer _renderer;
	stock::StockDataProviderPtr _stockDataProvider;
	stock::Inventory _inventory;

	attrib::ShadowAttributes _attrib;
	std::vector<std::string> _animations;
	std::vector<std::string> _items;
	int _itemIdx = 0;
	int _animationIdx = 0;

	io::FilePtr _luaFile;

	int _currentCharacterIndex = 0;
	const char *currentCharacter() const;
	bool loadCharacter();
	void doRender() override;
	void onRenderUI() override;
	bool addItem(stock::ItemId id);
public:
	TestAnimation(const metric::MetricPtr& metric, const stock::StockDataProviderPtr& stockDataProvider,
			const io::FilesystemPtr& filesystem,
			const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider,
			const animation::CharacterCachePtr& characterCache);

	core::AppState onConstruct() override;
	core::AppState onInit() override;
	core::AppState onCleanup() override;
};
