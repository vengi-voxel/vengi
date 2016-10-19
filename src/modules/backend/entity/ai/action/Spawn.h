/**
 * @file
 */

#pragma once

#include "Task.h"
#include "core/Common.h"
#include "backend/spawn/SpawnMgr.h"

using namespace ai;

namespace backend {

/**
 * @ingroup AI
 */
class Spawn: public Task {
private:
	backend::SpawnMgrPtr _spawnMgr;
public:
	class Factory: public ITreeNodeFactory {
	private:
		backend::SpawnMgrPtr _spawnMgr;
	public:
		Factory(const backend::SpawnMgrPtr& spawnMgr) :
				_spawnMgr(spawnMgr) {
		}

		TreeNodePtr create(const TreeNodeFactoryContext *ctx) const {
			return std::make_shared<Spawn>(ctx->name, ctx->parameters, ctx->condition, _spawnMgr);
		}
	};

	static Factory& getInstance(const backend::SpawnMgrPtr& spawnMgr) {
		AI_THREAD_LOCAL Factory FACTORY(spawnMgr);
		return FACTORY;
	}

	Spawn(const std::string& name, const std::string& parameters, const ConditionPtr& condition, const backend::SpawnMgrPtr& spawnMgr) :
			Task(name, parameters, condition), _spawnMgr(spawnMgr) {
	}

	TreeNodeStatus doAction(backend::AICharacter& chr, int64_t deltaMillis) override {
		backend::Npc& npc = chr.getNpc();
		const glm::ivec3 pos = glm::ivec3(npc.pos());
		if (_spawnMgr->spawn(*npc.ai()->getZone(), npc.entityType(), 1, &pos) == 1) {
			return FINISHED;
		}
		return FAILED;
	}
};

}

