/**
 * @file
 */

#pragma once

#include "ServerMessages_generated.h"
#include "backend/ForwardDecl.h"
#include "core/IComponent.h"
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>

namespace backend {

class SpawnMgr : public core::IComponent {
private:
	Map* _map;
	AILoaderPtr _loader;
	EntityStoragePtr _entityStorage;
	network::ServerMessageSenderPtr _messageSender;
	core::TimeProviderPtr _timeProvider;
	attrib::ContainerProviderPtr _containerProvider;
	cooldown::CooldownProviderPtr _cooldownProvider;
	io::FilesystemPtr _filesystem;
	long _time = 15000L;

	void spawnEntity(network::EntityType start, network::EntityType end, int maxAmount);
	void spawnAnimals();
	void spawnCharacters();

	NpcPtr createNpc(network::EntityType type, const TreeNodePtr& behaviour);
	bool onSpawn(const NpcPtr& npc, const glm::ivec3* pos);

public:
	SpawnMgr(Map* map,
			const io::FilesystemPtr& filesytem,
			const EntityStoragePtr& entityStorage,
			const network::ServerMessageSenderPtr& messageSender,
			const core::TimeProviderPtr& timeProvider,
			const AILoaderPtr& loader,
			const attrib::ContainerProviderPtr& containerProvider,
			const cooldown::CooldownProviderPtr& cooldownProvider);
	bool init() override;
	void shutdown() override;

	NpcPtr spawn(network::EntityType type, const glm::ivec3* pos = nullptr);
	int spawn(network::EntityType type, int amount, const glm::ivec3* pos = nullptr);
	void update(long dt);
};

typedef std::shared_ptr<SpawnMgr> SpawnMgrPtr;

}
