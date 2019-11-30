/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "animation/chr/Character.h"
#include "animation/AnimationCache.h"
#include "animation/AnimationRenderer.h"
#include "attrib/ShadowAttributes.h"
#include "stock/StockDataProvider.h"
#include "stock/Stock.h"
#include <vector>
#include <string>

/**
 * @brief Renders a character animation
 */
class TestAnimation: public TestApp {
private:
	using Super = TestApp;

	animation::Character _character;
	animation::AnimationCachePtr _animationCache;
	animation::AnimationRenderer _renderer;
	stock::StockDataProviderPtr _stockDataProvider;
	stock::Stock _stock;

	attrib::ShadowAttributes _attrib;
	std::vector<std::string> _animations;
	std::vector<std::string> _items;
	int _itemIdx = 0;
	int _animationIdx = 0;

	io::FilePtr _luaFile;

	int _currentCharacterIndex = 0;
	std::string currentCharacter() const;
	bool loadCharacter();
	void doRender() override;
	void onRenderUI() override;
	bool addItem(stock::ItemId id);
public:
	TestAnimation(const metric::MetricPtr& metric, const stock::StockDataProviderPtr& stockDataProvider,
			const io::FilesystemPtr& filesystem,
			const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider,
			const animation::AnimationCachePtr& animationCache);

	core::AppState onConstruct() override;
	core::AppState onInit() override;
	core::AppState onCleanup() override;
};
