/**
 * @file
 */

#include "TestAnimation.h"
#include "ui/imgui/IMGUI.h"
#include "core/io/Filesystem.h"
#include "voxel/MaterialColor.h"
#include "core/command/Command.h"
#include "core/Array.h"
#include "Shared_generated.h"
#include "animation/chr/Character.h"
#include <array>

static bool reloadAnimationEntity = false;

static std::vector<std::string> validCharacters;

TestAnimation::TestAnimation(const metric::MetricPtr& metric, const stock::StockDataProviderPtr& stockDataProvider,
		const io::FilesystemPtr& filesystem,
		const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider,
		const animation::AnimationCachePtr& animationCache) :
		Super(metric, filesystem, eventBus, timeProvider), _animationCache(
				animationCache), _stockDataProvider(stockDataProvider), _stock(stockDataProvider) {
	init(ORGANISATION, "testanimation");
	setCameraMotion(true);
	setRenderAxis(true);

	const int32_t cnt = (int)network::EntityType::MAX_CHARACTERS - ((int)network::EntityType::BEGIN_CHARACTERS + 1);
	validCharacters.resize(cnt);
	int index = 0;
	for (int i = ((int)network::EntityType::BEGIN_CHARACTERS) + 1; i < (int)network::EntityType::MAX_CHARACTERS; ++i) {
		const char *entityName = network::EnumNameEntityType((network::EntityType)i);
		std::string lower = core::string::toLower(core::string::format("chr/%s", entityName));
		core::string::replaceAllChars(lower, '_', '-');
		validCharacters[index++] = lower;
	}
}

const std::vector<std::string>& TestAnimation::animationEntityTypes() const {
	return validCharacters;
}

animation::AnimationEntity* TestAnimation::animationEntity() {
	static animation::Character _character;
	return &_character;
}

core::AppState TestAnimation::onConstruct() {
	core::AppState state = Super::onConstruct();

	core::Command::registerCommand("animation_cycle", [this] (const core::CmdArgs& argv) {
		int offset = 1;
		if (argv.size() > 0) {
			offset = core::string::toInt(argv[0]);
		}
		_animationIdx += offset;
		while (_animationIdx < 0) {
			_animationIdx += std::enum_value(animation::Animation::Max);
		}
		_animationIdx %= std::enum_value(animation::Animation::Max);
		Log::info("current animation idx: %i", _animationIdx);
		animationEntity()->setAnimation((animation::Animation)_animationIdx);
	});

	core::Command::registerCommand("animation_cycleenttype", [this] (const core::CmdArgs& argv) {
		int offset = 1;
		if (argv.size() > 0) {
			offset = core::string::toInt(argv[0]);
		}
		int current = (int)_entityType;
		current += offset;
		while (current < 0) {
			current += std::enum_value(EntityType::Max);
		}
		current %= (int)EntityType::Max;
		_entityType = (EntityType)current;
		loadAnimationEntity();
	});

	core::Command::registerCommand("animation_cycletype", [this] (const core::CmdArgs& argv) {
		int offset = 1;
		if (argv.size() > 0) {
			offset = core::string::toInt(argv[0]);
		}
		int& current = &_currentAnimationEntityIndex;
		const int size = animationEntityTypes().size();
		current += offset;
		while (current < 0) {
			current += size;
		}
		current %= size;
		Log::info("current animation entity idx: %i", current);
		loadAnimationEntity();
	});

	return state;
}

bool TestAnimation::loadAnimationEntity() {
	const int size = animationEntityTypes().size();
	_currentAnimationEntityIndex %= size;
	const std::string& ent = animationEntityTypes()[_currentAnimationEntityIndex];
	Log::info("Load animation entity settings for %s", ent.c_str());
	const io::FilePtr& file = filesystem()->open(animation::luaFilename(ent.c_str()));
	const std::string& lua = file->load();
	if (lua.empty()) {
		Log::error("Failed to load animation entity settings for %s", ent.c_str());
		return false;
	}

	animationEntity()->init(_animationCache, lua);
	if (_luaFile) {
		filesystem()->unwatch(_luaFile);
	}
	_luaFile = file;
	filesystem()->watch(_luaFile, [] (const char *file) {
		reloadAnimationEntity = true;
	});

	return true;
}

core::AppState TestAnimation::onInit() {
	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	for (int i = 0; i < std::enum_value(animation::Animation::Max); ++i) {
		_animations.push_back(animation::toString((animation::Animation)i));
	}

	_camera.setPosition(glm::vec3(10.0f, 5.0f, 10.0f));
	_camera.lookAt(glm::zero<glm::vec3>());

	if (!voxel::initDefaultMaterialColors()) {
		Log::error("Failed to initialize the default material colors");
		return core::AppState::InitFailure;
	}

	if (!_stockDataProvider->init(filesystem()->load("stock.lua"))) {
		Log::error("Failed to init stock data provider: %s", _stockDataProvider->error().c_str());
		return core::AppState::InitFailure;
	}

	if (!_stock.init()) {
		Log::error("Failed to init stock");
		return core::AppState::InitFailure;
	}

	const auto& items = _stockDataProvider->items();
	for (const auto& item : items) {
		if (item == nullptr) {
			break;
		}
		_items.push_back(item->name());
	}
	Log::info("Added %i items to the combo box", (int)_items.size());

	if (_items.empty()) {
		Log::error("Failed to load items");
		return core::AppState::InitFailure;
	}
	if (!_animationCache->init()) {
		Log::error("Failed to initialize the character mesh cache");
		return core::AppState::InitFailure;
	}

	if (!loadAnimationEntity()) {
		Log::error("Failed to initialize the animation entity");
		return core::AppState::InitFailure;
	}

	if (!_renderer.init()) {
		Log::error("Failed to initialize the character renderer");
		return core::AppState::InitFailure;
	}

	_attrib.setCurrent(attrib::Type::SPEED, 10.0);
	const animation::Animation currentAnimation = animationEntity()->animation();
	_animationIdx = std::enum_value(currentAnimation);

	const stock::ItemData* itemData = _stockDataProvider->itemData(_items[0]);
	if (!addItem(itemData->id())) {
		return core::AppState::InitFailure;
	}

	return state;
}

bool TestAnimation::addItem(stock::ItemId id) {
	const stock::ContainerData* containerData = _stockDataProvider->containerData("tool");
	if (containerData == nullptr) {
		return false;
	}
	const stock::ItemData* itemData = _stockDataProvider->itemData(id);
	if (itemData == nullptr) {
		Log::error("Failed to get item with id %i", (int)id);
		return false;
	}
	const stock::ItemPtr& item = _stockDataProvider->createItem(itemData->id());
	_stock.inventory().remove(containerData->id, 0, 0);
	if (!_stock.inventory().add(containerData->id, item, 0, 0)) {
		Log::error("Failed to add item to inventory");
		return false;
	}
	Log::info("Added item %s", itemData->name());
	return true;
}

void TestAnimation::doRender() {
	if (reloadAnimationEntity) {
		Log::info("Reload animation entity because file was modified");
		loadAnimationEntity();
		reloadAnimationEntity = false;
	}
	if (_entityType == EntityType::Character) {
		((animation::Character*)animationEntity())->updateTool(_animationCache, _stock);
	}
	animationEntity()->update(_deltaFrameMillis, _attrib);
	_renderer.render(*animationEntity(), _camera);
}

void TestAnimation::onRenderUI() {
	if (ImGui::Combo("EntityType", (int*)&_entityType, EntityTypeStrings, lengthof(EntityTypeStrings))) {
		loadAnimationEntity();
	}
	if (ImGui::ComboStl("Animation", &_animationIdx, _animations)) {
		animationEntity()->setAnimation((animation::Animation)_animationIdx);
	}
	if (_entityType == EntityType::Character) {
		if (ImGui::ComboStl("Item/Tool", &_itemIdx, _items)) {
			addItem(_itemIdx);
		}
	}
	if (ImGui::ComboStl("Entity", &_currentAnimationEntityIndex, animationEntityTypes())) {
		loadAnimationEntity();
	}
	Super::onRenderUI();
}

core::AppState TestAnimation::onCleanup() {
	core::AppState state = Super::onCleanup();
	_stock.shutdown();
	_animationCache->shutdown();
	_stockDataProvider->shutdown();
	_renderer.shutdown();
	return state;
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	const metric::MetricPtr& metric = std::make_shared<metric::Metric>();
	const animation::AnimationCachePtr& animationCache = std::make_shared<animation::AnimationCache>();
	const stock::StockDataProviderPtr& stockDataProvider = std::make_shared<stock::StockDataProvider>();
	TestAnimation app(metric, stockDataProvider, filesystem, eventBus, timeProvider, animationCache);
	return app.startMainLoop(argc, argv);
}
