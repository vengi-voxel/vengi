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
#include <array>

static bool reloadCharacter = false;

static constexpr int32_t cnt = (int)network::EntityType::MAX_CHARACTERS - ((int)network::EntityType::BEGIN_CHARACTERS + 1);
static std::array<std::string, cnt> validCharacters {};

TestAnimation::TestAnimation(const metric::MetricPtr& metric, const stock::StockDataProviderPtr& stockDataProvider,
		const io::FilesystemPtr& filesystem,
		const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider,
		const animation::AnimationCachePtr& animationCache) :
		Super(metric, filesystem, eventBus, timeProvider), _animationCache(
				animationCache), _stockDataProvider(stockDataProvider) {
	init(ORGANISATION, "testanimation");
	setCameraMotion(true);
	setRenderAxis(true);

	int index = 0;
	for (int i = ((int)network::EntityType::BEGIN_CHARACTERS) + 1; i < (int)network::EntityType::MAX_CHARACTERS; ++i) {
		const char *entityName = network::EnumNameEntityType((network::EntityType)i);
		std::string lower = core::string::toLower(core::string::format("chr/%s", entityName));
		core::string::replaceAllChars(lower, '_', '-');
		validCharacters[index++] = lower;
	}
}

std::string TestAnimation::currentCharacter() const {
	return validCharacters[_currentCharacterIndex];
}

core::AppState TestAnimation::onConstruct() {
	core::AppState state = Super::onConstruct();

	core::Command::registerCommand("cycle_animation", [this] (const core::CmdArgs& argv) {
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
		_character.setAnimation((animation::Animation)_animationIdx);
	});

	core::Command::registerCommand("cycle_character", [this] (const core::CmdArgs& argv) {
		int offset = 1;
		if (argv.size() > 0) {
			offset = core::string::toInt(argv[0]);
		}
		_currentCharacterIndex += offset;
		while (_currentCharacterIndex < 0) {
			_currentCharacterIndex += validCharacters.size();
		}
		_currentCharacterIndex %= validCharacters.size();
		Log::info("current character idx: %i", _currentCharacterIndex);
		loadCharacter();
	});

	return state;
}

bool TestAnimation::loadCharacter() {
	const std::string& chr = currentCharacter();
	Log::info("Load character settings for %s", chr.c_str());
	const io::FilePtr& file = filesystem()->open(animation::luaFilename(chr.c_str()));
	const std::string& lua = file->load();
	if (lua.empty()) {
		Log::error("Failed to load character settings for %s", chr.c_str());
		return false;
	}

	animation::Character testChr;
	if (!testChr.init(_animationCache, lua)) {
		Log::error("Failed to initialize the character %s for animation", chr.c_str());
		return false;
	}
	core_assert_always(_character.init(_animationCache, lua));
	if (_luaFile) {
		filesystem()->unwatch(_luaFile);
	}
	_luaFile = file;
	filesystem()->watch(_luaFile, [] (const char *file) {
		reloadCharacter = true;
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

	for (const auto& entry : _stockDataProvider->containers()) {
		const stock::ContainerData* data = entry.second;
		if (!_inventory.initContainer(data->id, data->shape, data->flags)) {
			Log::error("Failed to init inventory container with name '%s'", entry.first.c_str());
			return core::AppState::InitFailure;
		}
		Log::info("Initialized container %i with name %s", (int)data->id, entry.first.c_str());
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

	if (!loadCharacter()) {
		Log::error("Failed to initialize the character");
		return core::AppState::InitFailure;
	}

	if (!_renderer.init()) {
		Log::error("Failed to initialize the character renderer");
		return core::AppState::InitFailure;
	}

	_attrib.setCurrent(attrib::Type::SPEED, 10.0);
	const animation::Animation currentAnimation = _character.animation();
	_animationIdx = std::enum_value(currentAnimation);

	const stock::ItemData* itemData = _stockDataProvider->itemData(_items[0]);
	if (!addItem(itemData->id())) {
		return core::AppState::InitFailure;
	}

	return state;
}

bool TestAnimation::addItem(stock::ItemId id) {
	const stock::ContainerData* containerData = _stockDataProvider->containerData("weapon");
	if (containerData == nullptr) {
		Log::error("Failed to get container with name 'weapon'");
		return false;
	}
	const stock::ItemData* itemData = _stockDataProvider->itemData(id);
	if (itemData == nullptr) {
		Log::error("Failed to get item with id %i", (int)id);
		return false;
	}
	const stock::ItemPtr& item = _stockDataProvider->createItem(itemData->id());
	_inventory.remove(containerData->id, 0, 0);
	if (!_inventory.add(containerData->id, item, 0, 0)) {
		Log::error("Failed to add item to inventory");
		return false;
	}
	Log::info("Added item %s", itemData->name());
	return true;
}

void TestAnimation::doRender() {
	if (reloadCharacter) {
		Log::info("Reload character because file was modified");
		loadCharacter();
		reloadCharacter = false;
	}
	_character.updateTool(_animationCache, _inventory);
	_character.update(_deltaFrameMillis, _attrib);
	_renderer.render(_character, _camera);
}

void TestAnimation::onRenderUI() {
	if (ImGui::ComboStl("Animation", &_animationIdx, _animations)) {
		_character.setAnimation((animation::Animation)_animationIdx);
	}
	if (ImGui::ComboStl("Item/Tool", &_itemIdx, _items)) {
		addItem(_itemIdx);
	}
	if (ImGui::ComboStl("Character", &_currentCharacterIndex, validCharacters)) {
		loadCharacter();
	}
	Super::onRenderUI();
}

core::AppState TestAnimation::onCleanup() {
	core::AppState state = Super::onCleanup();
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
