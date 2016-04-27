#include "Idle.h"

namespace ai {

TreeNodePtr Idle::Factory::create(const TreeNodeFactoryContext *ctx) const {
	return TreeNodePtr(new Idle(ctx->name, ctx->parameters, ctx->condition));
}

Idle::Factory Idle::FACTORY;

}
