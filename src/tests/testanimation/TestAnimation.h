/**
 * @file
 */

#pragma once

#include "animation/AnimationSystem.h"
#include "testcore/TestApp.h"
#include "animation/AnimationCache.h"
#include "animation/AnimationRenderer.h"
#include "attrib/ShadowAttributes.h"
#include "stock/StockDataProvider.h"
#include "stock/Stock.h"
#include "core/ArrayLength.h"
#include <vector>
#include "core/String.h"
#include "io/File.h"

/**
 * @brief Renders a character animation
 */
class TestAnimation: public TestApp {
private:
	using Super = TestApp;

	int _entityType = 0;
	animation::AnimationCachePtr _animationCache;
	animation::AnimationRenderer _renderer;
	animation::AnimationSystem _animationSystem;
	stock::StockDataProviderPtr _stockDataProvider;
	stock::Stock _stock;

	attrib::ShadowAttributes _attrib;
	std::vector<core::String> _animations;
	std::vector<core::String> _items;
	int _itemIdx = 0;
	int _animationIdx = 0;

	io::FilePtr _luaFile;

	int _currentAnimationEntityIndex = 0;
	double _timeScale = 1.0;
	bool loadAnimationEntity();
	void doRender() override;
	void onRenderUI() override;
	bool addItem(stock::ItemId id);

	const std::vector<core::String>& animationEntityTypes() const;
	animation::AnimationEntity* animationEntity();
public:
	TestAnimation(const metric::MetricPtr& metric, const stock::StockDataProviderPtr& stockDataProvider,
			const io::FilesystemPtr& filesystem,
			const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider,
			const animation::AnimationCachePtr& animationCache);

	app::AppState onConstruct() override;
	app::AppState onInit() override;
	app::AppState onCleanup() override;
};
