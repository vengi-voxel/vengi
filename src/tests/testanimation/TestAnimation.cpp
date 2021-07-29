/**
 * @file
 */

#include "TestAnimation.h"
#include "ui/imgui/IMGUI.h"
#include "voxel/MaterialColor.h"
#include "Shared_generated.h"
#include "animation/Animation.h"
#include "animation/chr/Character.h"
#include "animation/animal/bird/Bird.h"
#include "stock/ContainerData.h"
#include "command/Command.h"
#include "core/ArrayLength.h"
#include "core/collection/Array.h"
#include "testcore/TestAppMain.h"
#include "voxelformat/MeshCache.h"

static bool reloadAnimationEntity = false;

static std::vector<core::String> validCharacters;
static std::vector<core::String> validBirds { "animal/animal-chicken" };

TestAnimation::TestAnimation(const metric::MetricPtr& metric, const stock::StockDataProviderPtr& stockDataProvider,
		const io::FilesystemPtr& filesystem,
		const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider,
		const animation::AnimationCachePtr& animationCache) :
		Super(metric, filesystem, eventBus, timeProvider), _animationCache(
				animationCache), _stockDataProvider(stockDataProvider), _stock(stockDataProvider) {
	init(ORGANISATION, "testanimation");
	setCameraMotion(false);
	setRenderAxis(true);

	const int32_t cnt = (int)network::EntityType::MAX_CHARACTERS - ((int)network::EntityType::BEGIN_CHARACTERS + 1);
	validCharacters.resize(cnt);
	int index = 0;
	for (int i = ((int)network::EntityType::BEGIN_CHARACTERS) + 1; i < (int)network::EntityType::MAX_CHARACTERS; ++i) {
		const char *entityName = network::EnumNameEntityType((network::EntityType)i);
		core::String lower = core::string::format("chr/%s", entityName).toLower();
		core::string::replaceAllChars(lower, '_', '-');
		validCharacters[index++] = lower;
	}
}

const std::vector<core::String>& TestAnimation::animationEntityTypes() const {
	if (_entityType == (int)animation::AnimationSettings::Type::Bird) {
		return validBirds;
	}
	return validCharacters;
}

animation::AnimationEntity* TestAnimation::animationEntity() {
	if (_entityType == (int)animation::AnimationSettings::Type::Bird) {
		static animation::Bird _bird;
		return &_bird;
	}
	static animation::Character _character;
	return &_character;
}

app::AppState TestAnimation::onConstruct() {
	app::AppState state = Super::onConstruct();

	command::Command::registerCommand("animation_cycle", [this] (const command::CmdArgs& argv) {
		int offset = 1;
		if (argv.size() > 0) {
			offset = core::string::toInt(argv[0]);
		}
		_animationIdx += offset;
		while (_animationIdx < 0) {
			_animationIdx += (core::enumVal(animation::Animation::MAX) + 1);
		}
		_animationIdx %= (core::enumVal(animation::Animation::MAX) + 1);
		Log::info("current animation idx: %i", _animationIdx);
		animationEntity()->setAnimation((animation::Animation)_animationIdx, true);
	});

	command::Command::registerCommand("animation_cycleenttype", [this] (const command::CmdArgs& argv) {
		int offset = 1;
		if (argv.size() > 0) {
			offset = core::string::toInt(argv[0]);
		}
		int& current = _entityType;
		current += offset;
		while (current < 0) {
			current += core::enumVal(animation::AnimationSettings::Type::Max);
		}
		current %= (int)animation::AnimationSettings::Type::Max;
		loadAnimationEntity();
	});

	command::Command::registerCommand("animation_cycletype", [this] (const command::CmdArgs& argv) {
		int offset = 1;
		if (argv.size() > 0) {
			offset = core::string::toInt(argv[0]);
		}
		int& current = _currentAnimationEntityIndex;
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
	const core::String& ent = animationEntityTypes()[_currentAnimationEntityIndex];
	Log::info("Load animation entity settings for %s", ent.c_str());
	const io::FilePtr& file = filesystem()->open(animation::luaFilename(ent.c_str()));
	const core::String& lua = file->load();
	if (lua.empty()) {
		Log::error("Failed to load animation entity settings for %s", ent.c_str());
		return false;
	}

	animationEntity()->init(_animationCache, lua);
	if (_luaFile) {
		filesystem()->unwatch(_luaFile);
	}
	_luaFile = file;
	static io::FileWatcher watcher { this, [] (void* userdata, const char *file) {
		reloadAnimationEntity = true;
	}};
	filesystem()->watch(_luaFile, &watcher);

	return true;
}

app::AppState TestAnimation::onInit() {
	app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

	if (!_animationSystem.init()) {
		Log::error("Failed to initialize the animation system");
		return app::AppState::InitFailure;
	}

	for (int i = core::enumVal(animation::Animation::MIN); i <= core::enumVal(animation::Animation::MAX); ++i) {
		_animations.push_back(animation::toString((animation::Animation)i));
	}

	camera().setPosition(glm::vec3(10.0f, 5.0f, 10.0f));
	camera().lookAt(glm::zero<glm::vec3>());

	if (!voxel::initDefaultMaterialColors()) {
		Log::error("Failed to initialize the default material colors");
		return app::AppState::InitFailure;
	}

	if (!_stockDataProvider->init(filesystem()->load("stock.lua"))) {
		Log::error("Failed to init stock data provider: %s", _stockDataProvider->error().c_str());
		return app::AppState::InitFailure;
	}

	if (!_stock.init()) {
		Log::error("Failed to init stock");
		return app::AppState::InitFailure;
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
		return app::AppState::InitFailure;
	}
	if (!_animationCache->init()) {
		Log::error("Failed to initialize the character mesh cache");
		return app::AppState::InitFailure;
	}

	if (!loadAnimationEntity()) {
		Log::error("Failed to initialize the animation entity");
		return app::AppState::InitFailure;
	}

	if (!_renderer.init()) {
		Log::error("Failed to initialize the character renderer");
		return app::AppState::InitFailure;
	}

	_attrib.setCurrent(attrib::Type::SPEED, 10.0);
	const animation::AnimationTimes& animations = animationEntity()->animations();
	for (int i = 0; i <= core::enumVal(animation::Animation::MAX); ++i) {
		if (animations[i] < _nowSeconds) {
			continue;
		}
		_animationIdx = i;
		break;
	}

	const stock::ItemData* itemData = _stockDataProvider->itemData(_items[0]);
	if (!addItem(itemData->id())) {
		return app::AppState::InitFailure;
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
	_renderer.setSeconds(_nowSeconds);

	if (reloadAnimationEntity) {
		Log::info("Reload animation entity because file was modified");
		loadAnimationEntity();
		reloadAnimationEntity = false;
	}
	if (animationEntity()->animationSettings().type() == animation::AnimationSettings::Type::Character) {
		((animation::Character*)animationEntity())->updateTool(_animationCache, _stock);
	}
	animationEntity()->update(_deltaFrameSeconds * _timeScale, _attrib);
	_renderer.render(*animationEntity(), camera());
}

void TestAnimation::onRenderUI() {
	if (ImGui::Combo("EntityType", &_entityType, animation::AnimationSettings::TypeStrings, lengthof(animation::AnimationSettings::TypeStrings))) {
		loadAnimationEntity();
	}
	if (ImGui::ComboStl("Animation", &_animationIdx, _animations)) {
		animationEntity()->setAnimation((animation::Animation)_animationIdx, true);
	}
	ImGui::InputDouble("Timescale", &_timeScale);
	if (animationEntity()->animationSettings().type() == animation::AnimationSettings::Type::Character) {
		if (ImGui::ComboStl("Item/Tool", &_itemIdx, _items)) {
			addItem(_itemIdx);
		}
	}
	static double speed = 0.0f;
	speed = _attrib.current(attrib::Type::SPEED);
	if (ImGui::InputDouble("Speed", &speed)) {
		_attrib.setCurrent(attrib::Type::SPEED, speed);
	}
	if (ImGui::ComboStl("Entity", &_currentAnimationEntityIndex, animationEntityTypes())) {
		loadAnimationEntity();
	}
	Super::onRenderUI();
}

app::AppState TestAnimation::onCleanup() {
	_stock.shutdown();
	_animationCache->shutdown();
	_stockDataProvider->shutdown();
	_renderer.shutdown();
	_animationSystem.shutdown();
	return Super::onCleanup();
}

int main(int argc, char *argv[]) {
	const voxelformat::MeshCachePtr& meshCache = core::make_shared<voxelformat::MeshCache>();
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	const metric::MetricPtr& metric = std::make_shared<metric::Metric>();
	const animation::AnimationCachePtr& animationCache = core::make_shared<animation::AnimationCache>(meshCache);
	const stock::StockDataProviderPtr& stockDataProvider = std::make_shared<stock::StockDataProvider>();
	TestAnimation app(metric, stockDataProvider, filesystem, eventBus, timeProvider, animationCache);
	return app.startMainLoop(argc, argv);
}
